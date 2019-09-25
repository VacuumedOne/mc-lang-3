//===----------------------------------------------------------------------===//
// Lexer
// このセクションでは、Lexerというテキストから「数字」や「演算子」をまとまり毎に
// 分割するクラスを実装している。
//
// 全体的な流れとしては、gettokをParserから呼ぶことにより「次のトークン」を読み、
// それが数値リテラルだった場合はnumValという変数にセットする。
//===----------------------------------------------------------------------===//

// このLexerでは、EOF、数値、"def"、識別子以外は[0-255]を返す('+'や'-'を含む)。
enum Token {
    tok_eof = -1,
    tok_def = -2,
    tok_identifier = -3,
    tok_int_number = -4,
    tok_double_number = -5,
    tok_if = -6,
    tok_then = -7,
    tok_else = -8,
    tok_op = -9,
    tok_int = -10,
    tok_double = -11
};

bool isNumberTok(Token t) {
    return (t == tok_int_number || t == tok_double_number);
}

std::set<char> op_char{
    '>',
    '<',
    '=',
    '+',
    '-',
    '*',
    '/',
    '!'
};

class NumberDFA {
    private:
    int state = 1; //初期状態0。状態は0から7まである
    std::string read_text = "";
    bool isE(int c) {
        return (c == 'E' || c == 'e');
    }
    bool isDot(int c) {
        return c == '.';
    }
    bool isPlus(int c) {
        return c == '+';
    }
    bool isMinus(int c) {
        return c == '-';
    }

    public:
    bool read(int c) { //一文字読んで遷移に成功したらtrue, 失敗したらfalse
        read_text += (char)c;
        switch(state) {
            case 1:
                if (isDot(c)) {
                    state = 2;
                    return true;
                } else if (isdigit(c)) {
                    state = 3;
                    return true;
                }
                break;
            case 2:
                if (isdigit(c)) {
                    state = 4;
                    return true;
                }
                break;
            case 3:
                if (isDot(c)) {
                    state = 4;
                    return true;
                } else if (isE(c)) {
                    state = 5;
                    return true;
                } else if (isdigit(c)) {
                    return true;
                }
                break;
            case 4:
                if (isE(c)) {
                    state = 5;
                    return true;
                } else if (isdigit(c)) {
                    return true;
                }
                break;
            case 5:
                if (isMinus(c) || isPlus(c)) {
                    state = 6;
                    return true;
                } else if (isdigit(c)) {
                    state = 7;
                    return true;
                }
                break;
            case 6:
                if (isdigit(c)) {
                    state = 7;
                    return true;
                }
                break;
            case 7:
                if (isdigit(c)) {
                    return true;
                }
                break;
        }
        return false;
    }
    bool isAccepted() {
        return (state == 3 || state == 4 || state == 7);
    }
    bool isInt() {
        return state == 3;
    }
    bool isDouble() {
        return (state == 4 || state == 7);
    }
    double getValue() {
        return strtod(read_text.c_str(), nullptr);
    }
};

class Lexer {
    public:
        // gettok - トークンが数値だった場合はnumValにその数値をセットした上でtok_number
        // を返し、トークンが識別子だった場合はidentifierStrにその文字をセットした上でtok_identifierを返す。
        // '+'や他のunknown tokenだった場合はそのascii codeを返す。
        int gettok() {
            static int lastChar = getNextChar(iFile);

            // スペースをスキップ
            while (isspace(lastChar))
                lastChar = getNextChar(iFile);

            // TODO 2.1: 識別子をトークナイズする
            // 1.3と同様に、今読んでいる文字がアルファベットだった場合はアルファベットで
            // なくなるまで読み込み、その値をidentifierStrにセットする。
            // 読み込んだ文字が"def"だった場合は関数定義であるためtok_defをreturnし、
            // そうでなければ引数の参照か関数呼び出しであるためtok_identifierをreturnする。
            if (isalpha(lastChar)) {
                identifierStr = lastChar;
                while (isalnum((lastChar = getNextChar(iFile))))
                    identifierStr += lastChar;

                if (identifierStr == "def")
                    return tok_def;
                if (identifierStr == "if")
                    return tok_if;
                if (identifierStr == "then")
                    return tok_then;
                if (identifierStr == "else")
                    return tok_else;
                if (identifierStr == "int")
                    return tok_int;
                if (identifierStr == "double")
                    return tok_double;
                // TODO 3.2: "if", "then", "else"をトークナイズしてみよう
                // 上記の"def"の例を参考に、3つの予約語をトークナイズして下さい。
                return tok_identifier;
            }

            // TODO 1.3: 数字のパーシングを実装してみよう
            // 今読んでいる文字(lastChar)が数字だった場合(isdigit(lastChar) == true)は、
            // 数字が終わるまで読み、その数値をnumValにセットする。
            // 1. lastCharが数字かチェックする e.g. if (isdigit(lastChar)) {..
            // 2. 数字を格納するための変数を作る e.g. std::string str = "";
            // 3. 入力が'456'という数字だったとして、今の文字は'4'なので、
            //    次の文字が数字ではなくなるまで読み込み、2で作った変数に格納する
            //    e.g. str += lastChar;
            // 4. 次の文字が数字でなくなるまで読み込む。ファイルから次の文字を読み込むのは、
            //    getNextChar(iFile)で出来る。 e.g. while(isdigit(lastChar = getNextChar(iFile))) {..
            // 5. 2と同様に、4が数字であれば2で作った変数に格納する
            // 6. 次の文字が数字でなくなったら、setnumValを読んでnumValに数値を格納する。その際、
            //   strtodというcharからintにする関数が利用できる。
            // 7. このトークンは数値だったので、tok_numberをreturnする。
            //
            // ここに実装して下さい

            //doubleのDFAを用いてdouble型数値をパースする
            if (isdigit(lastChar) || (lastChar=='.')) {
                NumberDFA dfa;
                while (dfa.read(lastChar)) { //読み込めなくなるまで読む
                    lastChar = getNextChar(iFile);
                }
                if (dfa.isAccepted()) { //受理状態か確認
                    if (dfa.isDouble()) {
                        setDoubleVal(dfa.getValue());
                        return tok_double_number;
                    } else {
                        setIntVal((int)dfa.getValue());
                        return tok_int_number;
                    }
                }
            }

            // TODO 1.4: コメントアウトを実装してみよう
            // '#'を読んだら、その行の末尾まで無視をするコメントアウトを実装する。
            // 1. 今の文字(LastChar)が'#'かどうかをチェック
            // 2. lastCharに次のトークンを読み込む(getNextChar(iFile)を使う)
            // 3. lastCharが'\n'か、EOFになるまで読み込む。e.g. while(lastChar != EOF && lastChar != '\n')
            // 4. lastCharがEOFでない場合、行末まで読み込んだので次のトークンに進むためにgettok()をreturnする。
            //
            // ここに実装して下さい
            if (lastChar == '#') {
                // Comment until end of line.
                lastChar = getNextChar(iFile);
                while (lastChar != EOF && lastChar != '\n')
                    lastChar = getNextChar(iFile);

                if (lastChar != EOF)
                    return gettok();
            }

            // EOFならtok_eofを返す
            if (iFile.eof())
                return tok_eof;
            
            // 演算子ならtok_opを返す
            if (op_char.count(lastChar) > 0) {
                std::string op = "";
                op += lastChar;
                lastChar = getNextChar(iFile);
                while(op_char.count(lastChar) > 0) {
                    op += lastChar;
                    lastChar = getNextChar(iFile);
                }
                setOperand(op);
                return tok_op;
            }

            // tok_numberでもtok_eofでもなければそのcharのasciiを返す
            int thisChar = lastChar;
            lastChar = getNextChar(iFile);
            return thisChar;
        }

        // 数字を格納するnumValのgetter, setter
        double getIntVal() { return intVal; }
        void setIntVal(int val) { intVal = val; }
        double getDoubleVal() { return doubleVal; }
        void setDoubleVal(double val) { doubleVal = val; }

        // 識別子を格納するIdentifierStrのgetter, setter
        std::string getIdentifier() { return identifierStr; }
        void setIdentifier(std::string str) { identifierStr = str; }

        // 演算子を格納するoperandStrのgetter, setter
        std::string getOperand() { return operandStr; }
        void setOperand(std::string str) { operandStr = str; } 

        void initStream(std::string fileName) { iFile.open(fileName); }

    private:
        std::ifstream iFile;
        int intVal;
        double doubleVal;
        std::string identifierStr;
        std::string operandStr;
        static char getNextChar(std::ifstream &is) {
            char c = '\0';
            if (is.good())
                is.get(c);

            return c;
        }
};
