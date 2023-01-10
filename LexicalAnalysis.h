/*****************************************************************//**
 * \file   LexicalAnalysis.h
 * \brief  
 * C���Դʷ�������
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
/// ��Ҫ�޸������׼���뺯��
void read_prog(string& prog)
{
	char c;
	while (scanf("%c", &c) != EOF) {
		if (c == '#')
			break;
		prog += c;
	}
}
/// ����������������
set<string> reserved = {
	"auto","break","case","char","const","continue",
	"default","do","double","else","enum","extern",
	"float","for","goto","if","int","long","register",
	"return","short","signed","sizeof","static","struct",
	"switch","typedef","union","unsigned","void","volatile",
	"while"
};
/**
 * �ĸ��ж�token���ַ����͵ĺ����������ַ�c��
 * ����ͨ�����ַ�������������������ζ��ַ������д���.
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

///token��Ӧ��ö������
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

///error��Ӧ��ö������
enum { INVALID_ID,INVALID_OPERATOR};

struct error {
	int type;	///��������
	string word;	///�����token����
	int line_num;	///token��λ��/����

	error() {}
	error(int t, string w, int ln) {
		type = t; word = w; line_num = ln;
	}
};
/**
 * �������࣬�ɸ��ã�
 * ��LexAnalyzerӵ�У���¼����ӡ������Ϣ
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
			cout << "������Ϣ : ��"<<e.line_num+1<<"��token\""<<e.word<<"\"����, ";
			switch (e.type) {
			case(INVALID_ID):
				cout << "��ʶ�����������ֿ�ͷ��Ӧ������ĸ��ͷ��"<<endl;
				break;
			case(INVALID_OPERATOR):
				cout << "���Ϸ�������������顣"<<endl;
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
 * ��������token���ɽ��ܸ����ͣ���������һ��token��ǰһ��λ�ã�
 * ��3��״̬���Զ�����������һ���ַ�����ת��״̬��ֱ����һ���ַ�Խ��򲻺Ϸ�.
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
 * ���������token����ע�ͣ�������һ��token��ǰһ��λ��.
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
		///"/"��ͷ��Ҫ���⴦��������ע��
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
 * ���ܱ����ֻ��߱�ʶ����������һ��token��ǰһ��λ��.
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
 * LexAnalyzer�ĳ�ʼ�������ñ����ֵȵ�token id��ӳ��.
 * 
 */
LexAnalyzer::LexAnalyzer() {
	tid = {
		{"auto",1},{"break",2},{"case",3},{"char",4},{"const",5},{"continue",6},{"default",7},{"do",8},{"double",9},{"else",10},{"enum",11},{"extern",12},{"float",13},{"for",14},{"goto",15},{"if",16},{"int",17},{"long",18},{"register",19},{"return",20},{"short",21},{"signed",22},{"sizeof",23},{"static",24},{"struct",25},{"switch",26},{"typedef",27},{"union",28},{"unsigned",29},{"void",30},{"volatile",31},{"while",32},{"-",33},{"--",34},{"-=",35},{"->",36},{"!",37},{"!=",38},{"%",39},{"%=",40},{"&",41},{"&&",42},{"&=",43},{"(",44},{")",45},{"*",46},{"*=",47},{",",48},{".",49},{"/",50},{"/=",51},{":",52},{";",53},{"?",54},{"[",55},{"]",56},{"^",57},{"^=",58},{"{",59},{"|",60},{"||",61},{"|=",62},{"}",63},{"~",64},{"+",65},{"++",66},{"+=",67},{"<",68},{"<<",69},{"<<=",70},{"<=",71},{"=",72},{"==",73},{">",74},{">=",75},{">>",76},{">>=",77},{"\"",78},{"Comment",79},{"Constant",80},{"Identifier",81}
	};
}
/**
 * ����������룬�������ʶ�𲢷ָ��token��
 * ���ݶ���token�����ַ���������token�ķ�ʽ��ֱ�����������
 * ��������к��д�����.
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

			///�������������token֮��û�пո����,�γɲ��Ϸ���ʶ���򲻺Ϸ�������ᱨ��
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
	/* ɧ���� �뿪ʼ���ǵı��� */
	/********* Begin *********/
	LexAnalyzer lexAnalyzer = LexAnalyzer();
	lexAnalyzer.execute(prog);
	///��ӡ������Ϣ
	lexAnalyzer.e.report();
	///��ӡ����token�Ľ��
	for (int i = 0; i < lexAnalyzer.res.size(); i++) {
		cout << i + 1 << ": <" << lexAnalyzer.res[i].word << "," << lexAnalyzer.res[i].id << ">" << endl;
	}
	/********* End *********/

}