//===----------------------------------------------------------------------===//
// AST
// ここのセクションでは、AST（構文解析木）の葉を定義している。
// MCコンパイラの根幹であるクラスで、ParserだけではなくCodeGenでも使われているので
// 非常に重要。イメージとしては、Lexerによって次のトークンを取ってきて、それが例えば
// 数値リテラルであったらNumberASTに値を格納し、そのポインタを親ノードが保持する。
// 全てのコードを無事にASTとして表現できたら、後述するcodegenを再帰的に呼び出す事に
// よりオブジェクトファイルを生成する。
//===----------------------------------------------------------------------===//
enum NumType {
    DEFAULT = -1, //codegenの段階でNumTypeが確定する。これはそれより前の仮の状態。
    INT = 0,
    DOUBLE = 1
};

NumType returnType(NumType type1, NumType type2) {
    if (type1 == DEFAULT || type2 == DEFAULT) {
        return DEFAULT;
    } else if (type1 == DOUBLE || type2 == DOUBLE) {
        return DOUBLE;
    } else {
        return INT;
    }
}

struct ArgTuple {
    std::string name;
    NumType type;
};

namespace {
    // ExprAST - `5+2`や`2*10-2`等のexpressionを表すクラス
    class ExprAST {
        public:
            virtual ~ExprAST() = default;
            virtual Value *codegen() = 0;
            NumType type = DEFAULT;
            virtual NumType checkType() {return DEFAULT;}
    };

    // NumberAST - `5`や`2`等の数値リテラルを表すクラス
    class NumberAST : public ExprAST {
        // 実際に数値の値を保持する変数
        int intVal;
        double doubleVal;

        public:
        NumberAST(double Val) {
            type = DOUBLE;
            doubleVal = Val;
        }
        NumberAST(int Val) {
            type = INT;
            intVal = Val;
        }
        NumType checkType() override {
            return type;
        }
        Value *codegen() override;
    };

    // BinaryAST - `+`や`*`等の二項演算子を表すクラス
    class BinaryAST : public ExprAST {
        std::string Op;
        std::unique_ptr<ExprAST> LHS, RHS;

        public:
        BinaryAST(std::string Op, std::unique_ptr<ExprAST> LHS,
                std::unique_ptr<ExprAST> RHS)
            : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
        NumType checkType() override {
            return returnType(LHS->checkType(), RHS->checkType());
        }
        Value *codegen() override;
    };

    // VariableExprAST - 変数の名前を表すクラス
    class VariableExprAST : public ExprAST {
        std::string variableName;

        public:
        VariableExprAST(const std::string &variableName) : variableName(variableName) {}
        NumType checkType() override;
        Value *codegen() override;
    };

    // CallExprAST - 関数呼び出しを表すクラス
    class CallExprAST : public ExprAST {
        std::string callee;
        std::vector<std::unique_ptr<ExprAST>> ArgList;

        public:
        CallExprAST(const std::string &callee,
                std::vector<std::unique_ptr<ExprAST>> ArgList)
            : callee(callee), ArgList(std::move(ArgList)) {}

        NumType checkType() override;
        Value *codegen() override;
    };

    // PrototypeAST - 関数シグネチャーのクラスで、関数の名前と引数の名前を表すクラス
    class PrototypeAST {
        std::string Name;
        std::vector<ArgTuple> ArgList;
        NumType type;

        public:
        PrototypeAST(const std::string &Name, std::vector<ArgTuple> ArgList, NumType type)
            : Name(Name), ArgList(std::move(ArgList)), type(type) {}

        Function *codegen();
        const std::string &getFunctionName() const { return Name; }
    };

    // FunctionAST - 関数シグネチャー(PrototypeAST)に加えて関数のbody(C++で言うint foo) {...}の中身)を
    // 表すクラスです。
    class FunctionAST {
        std::unique_ptr<PrototypeAST> proto;
        std::unique_ptr<ExprAST> body;

        public:
        FunctionAST(std::unique_ptr<PrototypeAST> proto,
                std::unique_ptr<ExprAST> body)
            : proto(std::move(proto)), body(std::move(body)) {}

        Function *codegen();
    };

    class IfExprAST : public ExprAST {
        std::unique_ptr<ExprAST> Cond, Then, Else;
        NumType type;

        public:
        IfExprAST(std::unique_ptr<ExprAST> Cond, std::unique_ptr<ExprAST> Then,
                std::unique_ptr<ExprAST> Else)
            : Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {}

        NumType checkType() override {
            return returnType(Then->checkType(), Else->checkType());
        }
        Value *codegen() override;
    };
} // end anonymous namespace

//===----------------------------------------------------------------------===//
// Parser
// Parserでは、ParseTopLevelExprから始まり、お互いを再帰的に呼び出すことでAST Tree(構文解析木)
// を構成していく。
//===----------------------------------------------------------------------===//

// CurTokは現在のトークン(tok_number, tok_eof, または')'や'+'などの場合そのascii)が
// 格納されている。
// getNextTokenにより次のトークンを読み、Curtokを更新する。
static int CurTok;
static int getNextToken() { return CurTok = lexer.gettok(); }

// 二項演算子の結合子をmc.cppで定義している。
static std::map<std::string, int> BinopPrecedence;

// GetTokPrecedence - 二項演算子の結合度を取得
// もし現在のトークンが二項演算子ならその結合度を返し、そうでないなら-1を返す。
static int GetTokPrecedence() {
    if (CurTok != tok_op)
        return -1;
    std::string op = lexer.getOperand();
    if (BinopPrecedence.count(op) == 0) 
        return -1;

    int tokprec = BinopPrecedence[op];
    if (tokprec <= 0)
        return -1;
    return tokprec;
}

// LogError - エラーを表示しnullptrを返してくれるエラーハンドリング関数
std::unique_ptr<ExprAST> LogError(const char *Str) {
    fprintf(stderr, "\e[31mError: %s\e[m\n", Str);
    return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char *Str) {
    fprintf(stderr, "Error: %s\n", Str);
    return nullptr;
}

// Forward declaration
static std::unique_ptr<ExprAST> ParseExpression();

// 数値リテラルをパースする関数。
static std::unique_ptr<ExprAST> ParseNumberExpr() {
    // NumberASTのValにlexerからnumValを読んできて、セットする。
    if (CurTok == tok_int_number) {
        auto Result = llvm::make_unique<NumberAST>((int)lexer.getIntVal());
        getNextToken(); // トークンを一個進めて、returnする。
        return std::move(Result);
    } else if (CurTok == tok_double_number){
        auto Result = llvm::make_unique<NumberAST>((double)lexer.getDoubleVal());
        getNextToken(); // トークンを一個進めて、returnする。
        return std::move(Result);
    } else {
        return LogError("This is NaN");
    }
}

// TODO 1.5: 括弧を実装してみよう
// 括弧は`'(' ExprAST ')'`の形で表されます。最初の'('を読んだ後、次のトークンは
// ExprAST(NumberAST or BinaryAST)のはずなのでそれをパースし、最後に')'で有ることを
// 確認します。
static std::unique_ptr<ExprAST> ParseParenExpr() {
    // 1. ParseParenExprが呼ばれた時、CurTokは'('のはずなので、括弧の中身を得るために
    //    トークンを進めます。e.g. getNextToken()
    // 2. 現在のトークンはExprASTのはずなので、ParseExpression()を呼んでパースします。
    //    もし返り値がnullptrならエラーなのでParseParenExprでもnullptrを返します。
    // 3. 2で呼んだParseExpressionではトークンが一つ進められて帰ってきているはずなので、
    // 　 CurTokが')'かどうかチェックします。もし')'でなければ、LogErrorを用いてエラーを出して下さい。
    // 4. getNextToken()を呼んでトークンを一つ進め、2で呼んだParseExpressionの返り値を返します。
    //
    // 課題を解く時はこの行を消してここに実装して下さい。
    getNextToken(); // eat (.
    auto V = ParseExpression();
    if (!V)
        return nullptr;

    if (CurTok != ')')
        return LogError("expected ')'");
    getNextToken(); // eat ).
    return V;
}

// TODO 2.2: 識別子をパースしよう
// トークンが識別子の場合は、引数(変数)の参照か関数の呼び出しの為、
// 引数の参照である場合はVariableExprASTを返し、関数呼び出しの場合は
// CallExprASTを返す。
static std::unique_ptr<ExprAST> ParseIdentifierExpr() {
    // 1. getIdentifierを用いて識別子を取得する。
    std::string IdName = lexer.getIdentifier();

    // 2. トークンを次に進める。
    getNextToken();

    // 3. 次のトークンが'('の場合は関数呼び出し。そうでない場合は、
    // VariableExprASTを識別子を入れてインスタンス化し返す。
    if (CurTok != '(')
        return llvm::make_unique<VariableExprAST>(IdName);

    // 4. '('を読んでトークンを次に進める。
    getNextToken();

    // 5. 関数fooの呼び出しfoo(3,4,5)の()内をパースする。
    // 引数は数値、二項演算子、(親関数で定義された)引数である可能性があるので、
    // ParseExpressionを用いる。
    // 呼び出しが終わるまで(CurTok == ')'になるまで)引数をパースしていき、都度argsにpush_backする。
    // 呼び出しの終わりと引数同士の区切りはCurTokが')'であるか','であるかで判別できることに注意。
    std::vector<std::unique_ptr<ExprAST>> args;
    if (CurTok != ')') {
        while (true) {
            if (auto val = ParseExpression())
                args.push_back(std::move(val));
            else
                return nullptr;

            if (CurTok == ')')
                break;

            if (CurTok != ',')
                return LogError("Expected ')' or ',' in argument list");
            getNextToken();
        }
    }

    // 6. トークンを次に進める。
    getNextToken();

    // 7. CallExprASTを構成し、返す。
    return llvm::make_unique<CallExprAST>(IdName, std::move(args));
}

static std::unique_ptr<ExprAST> ParseIfExpr() {
    // return nullptr;
    // TODO 3.3: If文のパーシングを実装してみよう。
    // 1. ParseIfExprに来るということは現在のトークンが"if"なので、
    // トークンを次に進めます。
    getNextToken();

    // 2. ifの次はbranching conditionを表すexpressionがある筈なので、
    // ParseExpressionを呼んでconditionをパースします。
    auto cond = ParseExpression();
    if (!cond) {
        return LogError("illegal condition expression after 'if'");
    }

    // 3. "if x < 4 then .."のような文の場合、今のトークンは"then"である筈なので
    // それをチェックし、トークンを次に進めます。
    if (CurTok != tok_then) {
        return LogError("no 'then' after 'if'.");
    }
    getNextToken();

    // 4. "then"ブロックのexpressionをParseExpressionを呼んでパースします。
    auto expr1 = ParseExpression();
    if (!expr1) {
        return LogError("no expression after 'if .. then'");
    }

    // 5. 3と同様、今のトークンは"else"である筈なのでチェックし、トークンを次に進めます。
    if (CurTok != tok_else) {
        return LogError("no 'else' after 'then ..'");
    }
    getNextToken();

    // 6. "else"ブロックのexpressionをParseExpressionを呼んでパースします。
    auto expr2 = ParseExpression();
    if (!expr2) {
        return LogError("no expression after 'else'");
    }
    // 7. IfExprASTを作り、returnします。
    return llvm::make_unique<IfExprAST>(std::move(cond), std::move(expr1), std::move(expr2));

}

// ParsePrimary - NumberASTか括弧をパースする関数
static std::unique_ptr<ExprAST> ParsePrimary() {
    switch (CurTok) {
        default:
            return LogError("unknown token when expecting an expression");
        case tok_identifier:
            return ParseIdentifierExpr();
        case tok_int_number:
        case tok_double_number:
            return ParseNumberExpr();
        case '(':
            return ParseParenExpr();
        case tok_if:
            return ParseIfExpr();
    }
}

// TODO 1.6: 二項演算のパーシングを実装してみよう
// このパーサーの中で一番重要と言っても良い、二項演算子のパーシングを実装します。
// LHSに二項演算子の左側が入った状態で呼び出され、LHSとRHSと二項演算子がペアになった
// 状態で返ります。
static std::unique_ptr<ExprAST> ParseBinOpRHS(int CallerPrec,
        std::unique_ptr<ExprAST> LHS) {
    while (true) {
        // 1. 現在の二項演算子の結合度を取得する。 e.g. int tokprec = GetTokPrecedence();
        int tokprec = GetTokPrecedence();

        // 2. もし呼び出し元の演算子(CallerPrec)よりも結合度が低ければ、ここでは演算をせずにLHSを返す。
        // 例えば、「4*2+3」ではここで'2'が返るはずで、「4+2*3」ではここでは返らずに処理が進み、
        // '2*3'がパースされた後に返る。
        if (tokprec < CallerPrec)
            return LHS;

        // 3. 二項演算子をセットする。e.g. int BinOp = CurTok;
        std::string BinOp = lexer.getOperand();

        // 4. 次のトークン(二項演算子の右のexpression)に進む。
        getNextToken();

        // 5. 二項演算子の右のexpressionをパースする。 e.g. auto RHS = ParsePrimary();
        auto RHS = ParsePrimary();
        if (!RHS)
            return nullptr;

        // GetTokPrecedence()を呼んで、もし次のトークンも二項演算子だった場合を考える。
        // もし次の二項演算子の結合度が今の演算子の結合度よりも強かった場合、ParseBinOpRHSを再帰的に
        // 呼んで先に次の二項演算子をパースする。
        int NextPrec = GetTokPrecedence();
        if (tokprec < NextPrec) {
            RHS = ParseBinOpRHS(tokprec + 1, std::move(RHS));
            if (!RHS)
                return nullptr;
        }

        // LHS, RHSをBinaryASTにしてLHSに代入する。
        LHS = llvm::make_unique<BinaryAST>(BinOp, std::move(LHS), std::move(RHS));
    }
}

// TODO 2.3: 関数のシグネチャをパースしよう
static std::unique_ptr<PrototypeAST> ParsePrototype() {
    // 2.2とほぼ同じ。CallExprASTではなくPrototypeASTを返し、
    // 引数同士の区切りが','ではなくgetNextToken()を呼ぶと直ぐに
    // CurTokに次の引数(もしくは')')が入るという違いのみ。
    NumType retType;
    if (CurTok == tok_int) {
        retType = INT;
    } else if (CurTok == tok_double) {
        retType = DOUBLE;
    } else {
        return LogErrorP("Expected type of return value");
    }
    getNextToken();

    if (CurTok != tok_identifier)
        return LogErrorP("Expected function name in prototype");

    std::string FnName = lexer.getIdentifier();;
    getNextToken();

    if (CurTok != '(')
        return LogErrorP("Expected '(' in prototype");

    std::vector<ArgTuple> ArgList;
    while (getNextToken() != ')') {
        if (CurTok == ',') {
            getNextToken();
        }
        NumType type;
        std::string name;
        if (CurTok == tok_int) {
            type = INT;
        } else if (CurTok == tok_double) {
            type = DOUBLE;
        } else {
            return LogErrorP("Expected type of argment");
        }
        if (getNextToken() == tok_identifier) {
            name = lexer.getIdentifier();
        }
        ArgTuple curArg = {name, type};
        ArgList.push_back(curArg);
    }
    if (CurTok != ')')
        return LogErrorP("Expected ')' in prototype");
    getNextToken();
    

    return llvm::make_unique<PrototypeAST>(FnName, std::move(ArgList), retType);
}

static std::unique_ptr<FunctionAST> ParseDefinition() {
    getNextToken();
    auto proto = ParsePrototype();
    if (!proto)
        return nullptr;

    if (auto E = ParseExpression())
        return llvm::make_unique<FunctionAST>(std::move(proto), std::move(E));
    return nullptr;
}

// ExprASTは1. 数値リテラル 2. '('から始まる演算 3. 二項演算子の三通りが考えられる為、
// 最初に1,2を判定して、そうでなければ二項演算子だと思う。
static std::unique_ptr<ExprAST> ParseExpression() {
    auto LHS = ParsePrimary();
    if (!LHS)
        return nullptr;

    return ParseBinOpRHS(0, std::move(LHS));
}

// パーサーのトップレベル関数。まだ関数定義は実装しないので、今のmc言語では
// __anon_exprという関数がトップレベルに作られ、その中に全てのASTが入る。
static std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
    if (auto E = ParseExpression()) {
        auto Proto = llvm::make_unique<PrototypeAST>("__anon_expr",
                std::vector<ArgTuple>(), INT);
        return llvm::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    }
    return nullptr;
}
