//===----------------------------------------------------------------------===//
// Code Generation
// このファイルでは、parser.hによって出来たASTからLLVM IRを生成しています。
// といっても、難しいことをしているわけではなく、IRBuilder(https://llvm.org/doxygen/IRBuilder_8h_source.html)
// のインターフェースを利用して、parser.hで定義した「ソースコードの意味」をIRに落としています。
// 各ファイルの中で一番LLVMの機能を多用しているファイルです。
//===----------------------------------------------------------------------===//

// https://llvm.org/doxygen/LLVMContext_8h_source.html
static LLVMContext Context;
// https://llvm.org/doxygen/classllvm_1_1IRBuilder.html
// LLVM IRを生成するためのインターフェース
static IRBuilder<> Builder(Context);
// https://llvm.org/doxygen/classllvm_1_1Module.html
// このModuleはC++ Moduleとは何の関係もなく、LLVM IRを格納するトップレベルオブジェクトです。
static std::unique_ptr<Module> myModule;

struct VariableTuple {
    Value *value;
    NumType type;
};
// 変数名とllvm::Valueのマップを保持する
static std::map<std::string, VariableTuple> NamedValues;

Type *cvtNumTypeToType(NumType nt) {
    Type *t;
    if (nt == INT) {
        t = Type::getInt64Ty(Context);
    } else if (nt == DOUBLE) {
        t = Type::getDoubleTy(Context);
    } else {
        LogError("type not found");
        t = nullptr;
    }
    return t;
}

NumType cvtTypeToNumType(Type *t) {
    NumType nt;
    if (t == Type::getInt64Ty(Context)) {
        nt = INT;
    } else if (t == Type::getDoubleTy(Context)) {
        nt = DOUBLE;
    } else {
        LogError("typenum not found");
        nt = DEFAULT;
    }
    return nt;
}


// https://llvm.org/doxygen/classllvm_1_1Value.html
// llvm::Valueという、LLVM IRのオブジェクトでありFunctionやModuleなどを構成するクラスを使います
Value *NumberAST::codegen() {
    // 64bit整数型のValueを返す
    if (type == INT) {
        return ConstantInt::get(Context, APInt(64, intVal, true));
    } else {
        return ConstantFP::get(Context, APFloat(doubleVal));
    }
}

Value *LogErrorV(const char *str) {
    LogError(str);
    return nullptr;
}

Function *LogErrorF(const char *str) {
    LogError(str);
    return nullptr;
}

// TODO 2.4: 引数のcodegenを実装してみよう
Value *VariableExprAST::codegen() {
    // NamedValuesの中にVariableExprAST::NameとマッチするValueがあるかチェックし、
    // あったらそのValueを返す。
    if (NamedValues.count(variableName) > 0) {
        VariableTuple vt = NamedValues[variableName];
        return vt.value;
    } else {
        return LogErrorV("Unknown variable name");
    }
}

NumType VariableExprAST::checkType() {
    if (NamedValues.count(variableName) > 0) {
        VariableTuple vt = NamedValues[variableName];
        return vt.type;
    } else {
        LogError("Unknown variable name");
        return DEFAULT;
    }
}

// TODO 2.5: 関数呼び出しのcodegenを実装してみよう
Value *CallExprAST::codegen() {
    // 1. myModule->getFunctionを用いてcalleeがdefineされているかを
    // チェックし、されていればそのポインタを得る。
    Function *CalleeF = myModule->getFunction(callee);
    if (!CalleeF)
        return LogErrorV("Unknown function referenced");

    int i = 0;
    std::vector<Value *> argsV;
    // 3. argsをそれぞれcodegenしllvm::Valueにし、argsVにpush_backする。
    for (auto &arg : CalleeF->args()) {
        Value *argV = ArgList[i]->codegen();

        NumType defType = cvtTypeToNumType(arg.getType()); //関数で定義されている型
        NumType argType = ArgList[i]->checkType(); //実際に呼び出しで指定された値の型
        if (defType != argType) {
            //型が一致しない場合
            return LogErrorV("cannot assign the value to this function");
        }

        argsV.push_back(argV);
        if (!argsV.back())
            return nullptr;
        i++;
    }

    // 4. IRBuilderのCreateCallを呼び出し、Valueをreturnする。
    return Builder.CreateCall(CalleeF, argsV, "calltmp");
}

NumType CallExprAST::checkType() {
    Function *CalleeF = myModule->getFunction(callee);
    if (!CalleeF) {
        LogError("Unknown function referenced");
        return DEFAULT;
    }
    Type *retType = CalleeF->getReturnType();
    return cvtTypeToNumType(retType);
}

Value *BinaryAST::codegen() {
    // 二項演算子の両方の引数をllvm::Valueにする。
    NumType type = checkType();

    NumType typeL = LHS->checkType();
    Value *L = LHS->codegen();

    NumType typeR = RHS->checkType();
    Value *R = RHS->codegen();

    if (!L || !R)
        return nullptr;
    
    //左右がIntとDoubleで食い違っている場合、Int側にDoubleへのキャストを施す
    //その予定だったけど、キャストがこれじゃダメらしいから断念。
    if (typeL == INT && type == DOUBLE) {
        return LogErrorV("cannot operate between 'int' and 'double'");
        // L = Builder.CreateFPCast(L, Type::getDoubleTy(Context), "cast_int_to_double");
    }
    if (typeR == INT && type == DOUBLE) {
        return LogErrorV("cannot operate between 'double' and 'int'");
        // R = Builder.CreateFPCast(R, Type::getDoubleTy(Context), "cast_int_to_double");
    }

    if (type == DOUBLE) {
        if (Op == "+") {
            return Builder.CreateFAdd(L, R, "double_add");
        } else if (Op == "-") {
            return Builder.CreateFSub(L, R, "double_sub");
        } else if (Op == "*") {
            return Builder.CreateFMul(L, R, "double_mul");
        } else if (Op == "/") {
            return Builder.CreateFDiv(L, R, "double_div");
        } else if (Op == "<") {
            L = Builder.CreateFCmpULT(L, R, "double_less_than");
            return Builder.CreateUIToFP(L, Type::getDoubleTy(Context), "bool_to_double");
        } else if (Op == ">") {
            L = Builder.CreateFCmpUGT(L, R, "double_greater_than");
            return Builder.CreateUIToFP(L, Type::getDoubleTy(Context), "bool_to_double");
        } else if (Op == "<=") {
            L = Builder.CreateFCmpULE(L, R, "double_equal_or_less_than");
            return Builder.CreateUIToFP(L, Type::getDoubleTy(Context), "bool_to_double");
        } else if (Op == ">=") {
            L = Builder.CreateFCmpUGE(L, R, "double_equal_or_greater_than");
            return Builder.CreateUIToFP(L, Type::getDoubleTy(Context), "bool_to_double");
        } else if (Op == "==") {
            L = Builder.CreateFCmpUEQ(L, R, "double_equal");
            return Builder.CreateUIToFP(L, Type::getDoubleTy(Context), "bool_to_double");
        } else if (Op == "!=") {
            L = Builder.CreateFCmpUNE(L, R, "double_not_equal");
            return Builder.CreateUIToFP(L, Type::getDoubleTy(Context), "bool_to_double");
        } else {
            return LogErrorV("invalid binary operator");
        }
    } else if (type == INT) {
        if (Op == "+") {
            return Builder.CreateAdd(L, R, "int_add");
        } else if (Op == "-") {
            return Builder.CreateSub(L, R, "int_sub");
        } else if (Op == "*") {
            return Builder.CreateMul(L, R, "int_mul");
        } else if (Op == "/") {
            return Builder.CreateSDiv(L, R, "int_div");
        } else if (Op == "<") {
            L = Builder.CreateICmpULT(L, R, "int_less_than");
            return Builder.CreateIntCast(L, Type::getInt64Ty(Context), true,  "bool_to_int");
        } else if (Op == ">") {
            L = Builder.CreateICmpUGT(L, R, "int_greater_than");
            return Builder.CreateIntCast(L, Type::getInt64Ty(Context), true,  "bool_to_int");
        } else if (Op == "<=") {
            L = Builder.CreateICmpULE(L, R, "int_equal_or_less_than");
            return Builder.CreateIntCast(L, Type::getInt64Ty(Context), true,  "bool_to_int");
        } else if (Op == ">=") {
            L = Builder.CreateICmpUGE(L, R, "int_equal_or_greater_than");
            return Builder.CreateIntCast(L, Type::getInt64Ty(Context), true,  "bool_to_int");
        } else if (Op == "==") {
            L = Builder.CreateICmpEQ(L, R, "int_equal");
            return Builder.CreateIntCast(L, Type::getInt64Ty(Context), true,  "bool_to_int");
        } else if (Op == "!=") {
            L = Builder.CreateICmpNE(L, R, "int_not_equal");
            return Builder.CreateIntCast(L, Type::getInt64Ty(Context), true,  "bool_to_int");
        } else {
            return LogErrorV("invalid binary operator");
        }
    } else {
        return LogErrorV("invalid type of return value");
    }
        
        // TODO 3.1: '<'を実装してみよう
        // '<'のcodegenを実装して下さい。その際、以下のIRBuilderのメソッドが使えます。
        // CreateICmp: https://llvm.org/doxygen/classllvm_1_1IRBuilder.html#a103d309fa238e186311cbeb961b5bcf4
        // llvm::CmpInst::ICMP_SLT: https://llvm.org/doxygen/classllvm_1_1CmpInst.html#a283f9a5d4d843d20c40bb4d3e364bb05
        // CreateIntCast: https://llvm.org/doxygen/classllvm_1_1IRBuilder.html#a5bb25de40672dedc0d65e608e4b78e2f
        // CreateICmpの返り値がi1(1bit)なので、CreateIntCastはそれをint64にcastするのに用います。
}

Function *PrototypeAST::codegen() {
    Type *retType = cvtNumTypeToType(type);

    std::vector<Type *> prototype;
    for (ArgTuple arg : ArgList) {
        NumType argNumType = arg.type;
        Type *argType = cvtNumTypeToType(arg.type);
        prototype.push_back(argType);
    }
    FunctionType *FT =
        FunctionType::get(retType, prototype, false);
    // https://llvm.org/doxygen/classllvm_1_1Function.html
    // llvm::Functionは関数のIRを表現するクラス
    Function *F =
        Function::Create(FT, Function::ExternalLinkage, Name, myModule.get());

    // 引数の名前を付ける
    unsigned i = 0;
    NamedValues.clear();
    for (auto &arg : F->args()) {
        arg.setName(ArgList[i].name);
        VariableTuple vp = {&arg, ArgList[i].type};
        NamedValues[ArgList[i].name] = vp;
        i++;
    }
    return F;
}

Function *FunctionAST::codegen() {
    // この関数が既にModuleに登録されているか確認
    Function *function = myModule->getFunction(proto->getFunctionName());
    // 関数名が見つからなかったら、新しくこの関数のIRクラスを作る。
    if (!function)
        function = proto->codegen();
    if (!function)
        return nullptr;

    // エントリーポイントを作る
    BasicBlock *BB = BasicBlock::Create(Context, "entry", function);
    Builder.SetInsertPoint(BB);

    // 関数のbody(ExprASTから継承されたNumberASTかBinaryAST)をcodegenする
    if (function->getReturnType() != cvtNumTypeToType(body->checkType())) { 
        //定義された返り値の型と、bodyの型が異なっていたらエラー
        LogError("illegal definition of function. defined type of return value is different from body type.");
        return nullptr;
    }
    Value *RetVal = body->codegen();
    if (RetVal) {
        // returnのIRを作る
        Builder.CreateRet(RetVal);

        // https://llvm.org/doxygen/Verifier_8h.html
        // 関数の検証
        verifyFunction(*function);

        return function;
    }

    // もし関数のbodyがnullptrなら、この関数をModuleから消す。
    function->eraseFromParent();
    return nullptr;
}

Value *IfExprAST::codegen() {
    NumType type = Cond->checkType();
    Value *CondV = Cond->codegen();
    if (!CondV)
        return nullptr;
    if (type == INT) {
        CondV = Builder.CreateICmpNE(CondV, ConstantInt::get(Context, APInt(64, 0)), "if_condition");
    } else if (type == DOUBLE) {
        CondV = Builder.CreateFCmpUNE(CondV, ConstantFP::get(Context, APFloat(0.0)), "if_condition");
    } else {
        return LogErrorV("illegal type of condition");
    }
    // if文を呼んでいる関数の名前
    Function *ParentFunc = Builder.GetInsertBlock()->getParent();

    // "thenだった場合"と"elseだった場合"のブロックを作り、ラベルを付ける。
    // "ifcont"はif文が"then"と"else"の処理の後、二つのコントロールフローを
    // マージするブロック。
    BasicBlock *ThenBB =
        BasicBlock::Create(Context, "then", ParentFunc);
    BasicBlock *ElseBB = BasicBlock::Create(Context, "else");
    BasicBlock *MergeBB = BasicBlock::Create(Context, "ifcont");
    // condition, trueだった場合のブロック、falseだった場合のブロックを登録する。
    // https://llvm.org/doxygen/classllvm_1_1IRBuilder.html#a3393497feaca1880ab3168ee3db1d7a4
    Builder.CreateCondBr(CondV, ThenBB, ElseBB);

    // "then"のブロックを作り、その内容(expression)をcodegenする。
    Builder.SetInsertPoint(ThenBB);
    NumType typeT = Then->checkType();
    if (typeT == INT && type == DOUBLE) {
        LogErrorV("Cannot convert 'double' to 'int'. Please set same type in THEN value and ELSE value.");
    }
    Value *ThenV = Then->codegen();
    if (!ThenV)
        return nullptr;
    // "then"のブロックから出る時は"ifcont"ブロックに飛ぶ。
    Builder.CreateBr(MergeBB);
    // ThenBBをアップデートする。
    ThenBB = Builder.GetInsertBlock();

    // TODO 3.4: "else"ブロックのcodegenを実装しよう
    // "then"ブロックを参考に、"else"ブロックのcodegenを実装して下さい。
    // 注意: 20行下のコメントアウトを外して下さい。
    ParentFunc->getBasicBlockList().push_back(ElseBB);
    Builder.SetInsertPoint(ElseBB);
    NumType typeE = Else->checkType();
    if (typeE == INT && type == DOUBLE) {
        LogErrorV("Cannot convert 'double' to 'int'. Please set same type in THEN value and ELSE value.");
    }
    Value *ElseV = Else->codegen();
    if (!ElseV)
        return nullptr;
    Builder.CreateBr(MergeBB);
    ElseBB = Builder.GetInsertBlock();


    // "ifcont"ブロックのcodegen
    ParentFunc->getBasicBlockList().push_back(MergeBB);
    Builder.SetInsertPoint(MergeBB);
    // https://llvm.org/docs/LangRef.html#phi-instruction
    // PHINodeは、"then"ブロックのValueか"else"ブロックのValue
    // どちらをifブロック全体の返り値にするかを実行時に選択します。
    // もちろん、"then"ブロックに入るconditionなら前者が選ばれ、そうでなければ後者な訳です。
    // LLVM IRはSSAという"全ての変数が一度だけassignされる"規約があるため、
    // 値を上書きすることが出来ません。従って、このように実行時にコントロールフローの
    // 値を選択する機能が必要です。
    PHINode *PN =
        Builder.CreatePHI(Type::getDoubleTy(Context), 2, "iftmp");

    PN->addIncoming(ThenV, ThenBB);
    // TODO 3.4:を実装したらコメントアウトを外して下さい。
    PN->addIncoming(ElseV, ElseBB);
 
    return PN;
}

//===----------------------------------------------------------------------===//
// MC コンパイラエントリーポイント
// mc.cppでMainLoop()が呼ばれます。MainLoopは各top level expressionに対して
// HandleTopLevelExpressionを呼び、その中でASTを作り再帰的にcodegenをしています。
//===----------------------------------------------------------------------===//

static std::string streamstr;
static llvm::raw_string_ostream stream(streamstr);
static void HandleDefinition() {
    if (auto FnAST = ParseDefinition()) {
        if (auto *FnIR = FnAST->codegen()) {
            FnIR->print(stream);
        }
    } else {
        getNextToken();
    }
}

// その名の通りtop level expressionをcodegenします。例えば、「2+1;3+3;」というファイルが
// 入力だった場合、この関数は最初の「2+1」をcodegenして返ります。(そしてMainLoopからまだ呼び出されます)
static void HandleTopLevelExpression() {
    // ここでテキストファイルを全てASTにします。
    if (auto FnAST = ParseTopLevelExpr()) {
        // できたASTをcodegenします。
        if (auto *FnIR = FnAST->codegen()) {
            streamstr = "";
            FnIR->print(stream);
        }
    } else {
        // エラー
        getNextToken();
    }
}

static void MainLoop() {
    myModule = llvm::make_unique<Module>("my cool jit", Context);
    while (true) {
        switch (CurTok) {
            case tok_eof:
                // ここで最終的なLLVM IRをプリントしています。
                fprintf(stderr, "%s", stream.str().c_str());
                return;
            case tok_def:
                HandleDefinition();
                break;
            case ';': // ';'で始まった場合、無視します
                getNextToken();
                break;
            default:
                HandleTopLevelExpression();
                break;
        }
    }
}
