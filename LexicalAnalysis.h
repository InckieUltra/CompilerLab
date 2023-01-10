/*****************************************************************//**
 * \file   LexicalAnalysis.h
 * \brief  
 * C语言词法分析器
 * \author Inckie
 * \date   January 2023
 *********************************************************************/
#include <cstdio>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>
using namespace std;
/// 不要修改这个标准输入函数
void read_prog(string& prog)
{
	char c;
	while (scanf("%c", &c) != EOF) {
		if (c == '#')
			break;
		prog += c;
	}
}
/// 你可以添加其他函数
set<string> reserved = {
	"auto","break","case","char","const","continue",
	"default","do","double","else","enum","extern",
	"float","for","goto","if","int","long","register",
	"return","short","signed","sizeof","static","struct",
	"switch","typedef","union","unsigned","void","volatile",
	"while"
};
/**
 * 四个判断token首字符类型的函数，接受字符c，
 * 后面通过首字符类型来决定接下来如何对字符串进行处理.
 * 
 * \param c
 * \return 
 */
bool isDigit(char c) {
	if (c >= '0' && c <= '9')
		return true;
	return false;
}
bool isAlpha(char c) {
	if (c >= 'a' && c <= 'z')
		return true;
	if (c >= 'A' && c <= 'Z')
		return true;
	if (c == '_')
		return true;
	return false;
}
bool isSign(char c) {
	if (c == '(' || c == ')' || c == '{' || c == '}' ||
		c == '[' || c == ']' || c == '"' || c == '\'' || c == ',' || c == '.' || c == ';' || c == ':')
		return true;
	return false;
}
bool isOperator(char c) {
	if (c == '<' || c == '=' || c == '>' || c == '+' || c == '-' || c == '*' || c == '/' || c == '^' || c == '!' || c == '%' ||
		c == '&' || c == '|' || c == '?' || c == '~')
		return true;
	return false;
}

///token对应的枚举类型
enum { NUM, ID, DELIMITER, QUOTE, OPERATOR, RESERVED };

struct token {
	string word;
	int type;
	int id;
	token() {}
	token(string w, int t, int idd) {
		word = w; type = t; id = idd;
	}
};

///error对应的枚举类型
enum { INVALID_ID,INVALID_OPERATOR};

struct error {
	int type;	///错误类型
	string word;	///错误的token内容
	int line_num;	///token的位置/计数

	error() {}
	error(int t, string w, int ln) {
		type = t; word = w; line_num = ln;
	}
};
/**
 * 错误处理类，可复用，
 * 由LexAnalyzer拥有，记录并打印错误信息
 */
class ErrorHandler {
private:
	vector<error> errors;
public:
	bool report();
	void add(int error, string word, int ln);
};

void ErrorHandler::add(int etype, string word, int ln) {
	errors.push_back(error(etype, word, ln));
}

bool ErrorHandler::report() {
	if (errors.empty()) {
		return false;
	}
	else {
		for (auto e : errors) {
			cout << "错误信息 : 第"<<e.line_num+1<<"个token\""<<e.word<<"\"出错, ";
			switch (e.type) {
			case(INVALID_ID):
				cout << "标识符不能以数字开头，应当以字母开头。"<<endl;
				break;
			case(INVALID_OPERATOR):
				cout << "不合法的运算符，请检查。"<<endl;
				break;
			default:
				cout << "unkown error:"<<e.type << endl;
			}
		}
		return true;
	}
}

class LexAnalyzer {
public:
	ErrorHandler e;
	string input;
	map<string, int> tid;
	LexAnalyzer();
	vector<token> res;
	void execute(string input);
	int get_num(int pos, string word);
	int get_operator(int pos, string word);
	int get_alpha(int pos, string word);
};
/**
 * 接受数字token（可接受浮点型），返回下一个token的前一个位置，
 * 有3个状态的自动机，接收下一个字符并且转换状态，直到下一个字符越界或不合法.
 * 
 * \param pos
 * \param word
 * \return 
 */
int LexAnalyzer::get_num(int pos, string word) {
	int j = pos;
	int state = 0;
	while (j + 1 < input.length()) {
		j++;
		if (isDigit(input[j]) && state == 0)
			;
		else if (input[j] == '.'&&state == 0) {
			state = 1;
		}
		else if (isDigit(input[j]) && state == 1)
			;
		else
			break;
		word += input[j];
	}
	res.push_back(token(word, NUM, 80));
	return pos + word.length() - 1;
}
/**
 * 接受运算符token或者注释，返回下一个token的前一个位置.
 * 
 * \param pos
 * \param word
 * \return 
 */
int LexAnalyzer::get_operator(int pos, string word) {
	int state = 0;
	int i = pos;
	if (i + 1 >= input.size()) {
		//do nothing
	}
	else if (input[i + 1] != ' '&&input[i + 1] != '\n'&&input[i + 1] != '\t') {
		switch (input[i]) {
		case '-':
			if (input[i + 1] == '-' || input[i + 1] == '=' || input[i + 1] == '>')
				word += input[i + 1];
			break;
		case '!':
			if (input[i + 1] == '=')
				word += input[i + 1];
			break;
		case '%':
			if (input[i + 1] == '=')
				word += input[i + 1];
			else if (isAlpha(input[i + 1])) {
				word += input[i + 1];
				res.push_back(token(word, ID, 81));
				return pos + word.length() - 1;
			}
			break;
		case '&':
			if (input[i + 1] == '&' || input[i + 1] == '=')
				word += input[i + 1];
			break;
		case '*':
			if (input[i + 1] == '=')
				word += input[i + 1];
			break;
		case '?':
			break;
		case '^':
			if (input[i + 1] == '=')
				word += input[i + 1];
			break;
		case '|':
			if (input[i + 1] == '|' || input[i + 1] == '=')
				word += input[i + 1];
			break;
		case '~':
			break;
		case '+':
			if (input[i + 1] == '+' || input[i + 1] == '=')
				word += input[i + 1];
			break;
		case '<':
			if (input[i + 1] == '<') {
				if (input[i + 2] == '=') {
					word = "<<=";
					break;
				}
				word = "<<";
				break;
			}
			if (input[i + 1] == '=')
				word += input[i + 1];
			break;
		case '=':
			if (input[i + 1] == '=')
				word += input[i + 1];
			break;
		case '>':
			if (input[i + 1] == '>') {
				if (input[i + 2] == '=') {
					word = ">>=";
					break;
				}
				word = ">>";
				break;
			}
			if (input[i + 1] == '=')
				word += input[i + 1];
			break;
		///"/"开头需要特殊处理，可能是注释
		case '/':
			if (input[i + 1] == '/') {
				int j = i;
				while (input[j + 1] != '\n') {
					j++;
					word += input[j];
				}
				res.push_back(token(word, OPERATOR, 79));
				return pos + word.length() - 1;
			}
			if (input[i + 1] == '*') {
				int j = i;
				j++;
				while (!(input[j] == '*'&&input[j + 1] == '/'))
					word += input[j++];
				word += '*'; word += '/';
				res.push_back(token(word, OPERATOR, 79));
				return pos + word.length() - 1;
			}
			if (input[i + 1] == '=')
				word += input[i + 1];
			break;
		}
	}
	res.push_back(token(word, OPERATOR, tid[word]));
	return pos + word.length() - 1;
}
/**
 * 接受保留字或者标识符，返回下一个token的前一个位置.
 * 
 * \param pos
 * \param word
 * \return 
 */
int LexAnalyzer::get_alpha(int pos, string word) {
	int j = pos;
	int state = 0;
	while (j + 1 < input.length()) {
		if (isDigit(input[j + 1]) || isAlpha(input[j + 1])) {
			j++; 
			word += input[j];
		}
		else
			break;
	}
	if (reserved.count(word))
		res.push_back(token(word, RESERVED, tid[word]));
	else
		res.push_back(token(word, ID, 81));
	return pos + word.length() - 1;
}
/**
 * LexAnalyzer的初始化，设置保留字等到token id的映射.
 * 
 */
LexAnalyzer::LexAnalyzer() {
	tid = {
		{"auto",1},{"break",2},{"case",3},{"char",4},{"const",5},{"continue",6},{"default",7},{"do",8},{"double",9},{"else",10},{"enum",11},{"extern",12},{"float",13},{"for",14},{"goto",15},{"if",16},{"int",17},{"long",18},{"register",19},{"return",20},{"short",21},{"signed",22},{"sizeof",23},{"static",24},{"struct",25},{"switch",26},{"typedef",27},{"union",28},{"unsigned",29},{"void",30},{"volatile",31},{"while",32},{"-",33},{"--",34},{"-=",35},{"->",36},{"!",37},{"!=",38},{"%",39},{"%=",40},{"&",41},{"&&",42},{"&=",43},{"(",44},{")",45},{"*",46},{"*=",47},{",",48},{".",49},{"/",50},{"/=",51},{":",52},{";",53},{"?",54},{"[",55},{"]",56},{"^",57},{"^=",58},{"{",59},{"|",60},{"||",61},{"|=",62},{"}",63},{"~",64},{"+",65},{"++",66},{"+=",67},{"<",68},{"<<",69},{"<<=",70},{"<=",71},{"=",72},{"==",73},{">",74},{">=",75},{">>",76},{">>=",77},{"\"",78},{"Comment",79},{"Constant",80},{"Identifier",81}
	};
}
/**
 * 接受输入代码，将代码段识别并分割成token，
 * 根据读到token的首字符决定接收token的方式，直到输入结束，
 * 这个函数中含有错误处理.
 * 
 * \param raw_input
 */
void LexAnalyzer::execute(string raw_input) {
	input = raw_input;
	string word;
	bool pre_null = true;
	for (int i = 0; i < input.length(); i++) {

		if (input[i] != ' '&&input[i] != '\n'&&input[i] != '\t') {
			pre_null = false;
			word = "";
			word += input[i];

			if (isDigit(input[i]))
				i = get_num(i, word);

			else if (isOperator(input[i]))
				i = get_operator(i, word);

			else if (isSign(input[i]))
				res.push_back(token(word, DELIMITER, tid[word]));

			else if (isAlpha(input[i]))
				i = get_alpha(i, word);

			///错误处理（如果两个token之间没有空格隔开,形成不合法标识符或不合法运算符会报错）
			if (pre_null == false&&res.size()>=2) {
				if (res[res.size() - 2].type == NUM && res[res.size() - 1].type == ID)
					e.add(INVALID_ID, res[res.size() - 2].word+res[res.size() - 1].word, res.size() - 2);
				else if (res[res.size() - 2].type == OPERATOR && res[res.size() - 1].type == OPERATOR)
					e.add(INVALID_OPERATOR, res[res.size() - 2].word + res[res.size() - 1].word, res.size() - 2);
			}

		}
		else
			pre_null = true;
	}
}

void Analysis()
{
	string prog;
	read_prog(prog);
	/* 骚年们 请开始你们的表演 */
	/********* Begin *********/
	LexAnalyzer lexAnalyzer = LexAnalyzer();
	lexAnalyzer.execute(prog);
	///打印错误信息
	lexAnalyzer.e.report();
	///打印分析token的结果
	for (int i = 0; i < lexAnalyzer.res.size(); i++) {
		cout << i + 1 << ": <" << lexAnalyzer.res[i].word << "," << lexAnalyzer.res[i].id << ">" << endl;
	}
	/********* End *********/

}