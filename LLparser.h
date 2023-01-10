/*****************************************************************//**
 * \file   LLparser.h
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
#include <deque>
using namespace std;
///不要修改这个标准输入函数
void read_prog(string &prog) {
	char c;
	while (scanf("%c", &c) != EOF) {
		if (c == '#')
			break;
		prog += c;
	}
}
///你可以添加其他函数

///token对应的枚举类型
enum { UNEXPECTED_SYMBOL,EARLY_EOF };
/**
 * token结构体，主要使用字符串和行号两个属性.
 */
struct token {
	string str;
	double value;
	int type;
	int line_num;

	token() {}
	token(string s, double v, int t, int ln) {
		str = s; value = v; type = t; line_num = ln;
	}
};

struct error {
	int type;
	string word;
	int line_num;

	error() {}
	error(int t, string w, int ln) {
		type = t; word = w; line_num = ln;
	}
};

vector<token> split_word(string s);
/**
 * 错误处理类，可复用，
 * 用于记录和打印错误信息，
 * 被LLparser类拥有.
 */
class ErrorHandler {
private:
	vector<error> errors;
public:
	bool report();
	void add(int error,string word, int ln);
};

void ErrorHandler::add(int etype,string word, int ln) {
	errors.push_back(error(etype,word, ln));
}

bool ErrorHandler::report() {
	if (errors.empty()) {
		return false;
	}
	else {
		for (auto e : errors) {
			cout << "语法错误,第" << e.line_num << "行,";
			switch (e.type) {
			case UNEXPECTED_SYMBOL:
				cout<<"缺少\""<<e.word<<"\""<<endl;
				break;
			case EARLY_EOF:
				cout << "这里不应该是EOF,你的代码不完整,请检查"<<endl;
				break;
			default:
				cout << "unkown error" << endl;
			}
		}
		return true;
	}
}
/**
 * Generator类，接受规范的文法（把"|"转换成多条产生式，按行分割，E表示空），
 * 自动生成LL(1) parsing table.
 */
class Generator {
public:
	vector< vector<string>> productions;
	set<string> term;
	set<string> nonterm;
	map<string, bool> nullable;
	map<string, int> null_pid;
	vector< set<string>> first;
	vector< set<string>> follow;
	vector< vector<int>> table;
	map<string, int> sid;
	Generator();
	Generator(string raw_rules);
	void generate_dic();
	void generate_first();
	void generate_follow();
	void generate_LLtable();
};

Generator::Generator() {}
/**
 * 将原始的rule按行分割并保存
 * 
 * \param raw_rules
 */
Generator::Generator(string raw_rules) {
	vector<token> temp = split_word(raw_rules);
	vector<string> production;
	int curline = -1, pos=0;
	while (pos < temp.size()) {
		production.clear();
		nonterm.insert(temp[pos].str);
		production.push_back(temp[pos].str);
		curline = temp[pos].line_num;
		pos += 2;
		if (pos >= temp.size())
			break;
		while (temp[pos].line_num == curline) {
			production.push_back(temp[pos].str);
			if (++pos >= temp.size())
				break;
		}
		productions.push_back(production);
	}
}
/**
 * 生成字典，记录终结符和非终结符，
 * 同时将直接产生空的非终结符记为nullable，
 * 并记录空产生式对应的产生式id.
 * 
 */
void Generator::generate_dic() {
	int cnt = 0; set<string> empty_set = {};
	for (auto nt : nonterm) {
		sid[nt] = cnt++;
		nullable[nt] = false;
		first.push_back(empty_set);
		follow.push_back(empty_set);
	}
	term.insert("$"); term.insert("E");
	int pid = -1;
	for (auto ps : productions) {
		pid++;
		for (auto p : ps) {
			if (!nonterm.count(p)&&p!="E")
				term.insert(p);
		}
		if (ps.size() == 2 && ps[1] == "E") {
			nullable[ps[0]] = true;
			null_pid[ps[0]] = pid;
		}
	}
	for (auto t : term)
		sid[t] = cnt++;
	vector<int> row;
	for (int i = 0; i < term.size() + nonterm.size() + 1; i++)
		row.push_back(-1);
	for (int i = 0; i < nonterm.size() + 1; i++)
		table.push_back(row);
}
/**
 * 先确定所有终结符是否为nullable，
 * 然后生成first集并根据first集完成一部分LL(1) table.
 * 
 */
void Generator::generate_first() {
	for (int i = 0; i < nonterm.size(); i++) {
		int pid = -1;
		for (auto ps : productions) {
			pid++;
			bool go_nullable = true;
			for (int j = 1; j < ps.size(); j++) {
				if(term.count(ps[j])||!nullable[ps[j]])
					go_nullable=false;
			}
			if (go_nullable) {
				nullable[ps[0]] = true;
				null_pid[ps[0]] = pid;
			}
		}
	}
	for (int i = 0; i < nonterm.size(); i++) {
		int pid = -1;
		for (auto ps : productions) {
			pid++;
			
			int j = 0;
			do {
				j++;
				if (j >= ps.size())
					break;
				if (term.count(ps[j])) {
					if (ps[j] == "E")
						continue;
					table[sid[ps[0]]][sid[ps[j]]] = pid;
					first[sid[ps[0]]].insert(ps[j]);
					break;
				}
				
				for (auto f : first[sid[ps[j]]]) {
					first[sid[ps[0]]].insert(f);
					table[sid[ps[0]]][sid[f]] = pid;
				}
			} while (nullable[ps[j]]||ps[j]=="E");
		}
	}
}
/**
 * 生成follow集并进一步完成全部的LL(1) table.
 * 
 */
void Generator::generate_follow() {
	table[sid[productions[0][0]]][sid["$"]] = 0;
	follow[sid[productions[0][0]]].insert("$");
	for (int i = 0; i < nonterm.size(); i++) {
		int pid = -1;
		for (auto ps:productions) {
			pid++;
			bool end_null = true;
			for (int j = ps.size() - 1; j >= 2; j--) {
				if (nonterm.count(ps[j - 1])) {
					
					if (term.count(ps[j])) {
						follow[sid[ps[j - 1]]].insert(ps[j]);
						if (nullable[ps[j - 1]])
							table[sid[ps[j - 1]]][sid[ps[j]]] = null_pid[ps[j-1]];
					}
					else {
						for (auto f : first[sid[ps[j]]]){
							follow[sid[ps[j - 1]]].insert(f);
							if (nullable[ps[j - 1]])
								table[sid[ps[j - 1]]][sid[f]] = null_pid[ps[j-1]];
						}
					}
				}
				if (nonterm.count(ps[j])&&end_null) {
					for (auto f : follow[sid[ps[0]]]) {
						follow[sid[ps[j]]].insert(f);
						if (nullable[ps[j]])						
							table[sid[ps[j]]][sid[f]] = null_pid[ps[j]];
					}
				}
				if (!nullable[ps[j]])
					end_null = false;
			}
			if (nonterm.count(ps[1]) && end_null) {
				for (auto f : follow[sid[ps[0]]]) {
					follow[sid[ps[1]]].insert(f);
					if (nullable[ps[1]])
						table[sid[ps[1]]][sid[f]] = null_pid[ps[1]];
				}
			}
		}
	}
}

void Generator::generate_LLtable() {
	generate_dic();
	generate_first();
	generate_follow();
}

class LLparser {
private:
	deque<token> input;
	deque<token> work;
	deque<int> depth;
	string parse_res;
	vector<token> tokens;
	ErrorHandler e;
	Generator g;
public:
	LLparser();
	void init_table(string rules);
	void execute(string input);
};

LLparser::LLparser() {}

void LLparser::init_table(string raw_rules) {
	
	g = Generator(raw_rules);
	g.generate_LLtable();

}
/**
 * 分词函数，将原始输入分割成带有行号信息的token.
 * 
 * \param s
 * \return 
 */
vector<token> split_word(string s) {
	vector<string> split_temp;
	vector<token> split_res;
	split_res.clear();
	string line = "";
	string word = "";
	for (auto c : s) {
		if (c == '\n') {
			if (line != "")
				split_temp.push_back(line);
			line = "";
		}
		else
			line += c;
	}
	if (line != "")
		split_temp.push_back(line);
	for (int i = 0; i < split_temp.size(); i++) {
		word = "";
		for (auto c : split_temp[i]) {
			if (c == ' ') {
				if (word != "")
					split_res.push_back(token(word, -1, -1, i + 1));
				word = "";
			}
			else
				word += c;
		}
		if (word != "")
			split_res.push_back(token(word, -1, -1, i + 1));
	}
	return split_res;
}
/**
 * 语法处理函数，
 * 接收对应代码，先分词，
 * 用deque模拟栈，进行Top-down的语法处理.
 * 
 * \param s
 */
void LLparser::execute(string s) {
	vector<token> split_res = split_word(s);
	for (int i = split_res.size() - 1; i >= 0; i--)
		input.push_front(split_res[i]);
	work.push_front(token("program", -1, -1, -1));
	depth.push_front(0);
	int curline = 1;
	int depthhead;
	token workhead, inputhead;
	string old_parse_res;
	while (!input.empty()) {
		old_parse_res = parse_res;
		workhead = work.front();
		inputhead = input.front();
		depthhead = depth.front();
		work.pop_front();
		depth.pop_front();
		for (int i = 0; i < depthhead; i++)
			parse_res += '\t';
		parse_res += workhead.str;
		parse_res += '\n';
		//cout << work.size() << ' ' << input.size() << ' ' << endl;
		//cout << workhead.str << ' ' << inputhead.str << ' ' <<  endl;
		if (g.term.count(workhead.str) && workhead.str == inputhead.str) {
			input.pop_front();
			curline = inputhead.line_num;
			//do nothing
		}
		else if (g.nonterm.count(workhead.str) && g.table[g.sid[workhead.str]][g.sid[inputhead.str]] != -1) {
			int pid = g.table[g.sid[workhead.str]][g.sid[inputhead.str]];
			vector<string> p = g.productions[pid];
			if (p.size() == 2 && p[1] == "E") {
				for (int i = 0; i < depthhead + 1; i++)
					parse_res += '\t';
				parse_res += "E\n";
			}
			else {
				for (int i = p.size() - 1; i >= 1; i--) {
					work.push_front(token(p[i],-1,-1,curline));
					depth.push_front(depthhead + 1);
				}
			}
		}
		///发生错误，修改input栈使得可以语法处理可以进行，同时回滚操作
		else {
			if (g.term.count(workhead.str)) {
				e.add(UNEXPECTED_SYMBOL, workhead.str, curline);
				input.push_front(token(workhead.str, -1, -1, curline));
			}
			else if (g.nullable[workhead.str]) {
				for (int i = 0; i < depthhead + 1; i++)
					parse_res += '\t';
				parse_res += "E\n";
				continue;
			}
			work.push_front(token(workhead.str, -1, -1, curline));
			depth.push_front(depthhead);
			parse_res = old_parse_res;
		}
	}
	if (!work.empty())
		e.add(EARLY_EOF,"", curline);
	e.report();
	cout << parse_res;
}

void Analysis() {
	string prog;
	read_prog(prog);
	/* 骚年们 请开始你们的表演 */
	/********* Begin *********/
	LLparser llp = LLparser();
	llp.init_table(R"(program -> compoundstmt
stmt -> ifstmt
stmt -> whilestmt
stmt -> assgstmt
stmt -> compoundstmt
compoundstmt -> { stmts }
stmts -> stmt stmts
stmts -> E
ifstmt -> if ( boolexpr ) then stmt else stmt
whilestmt -> while ( boolexpr ) stmt
assgstmt -> ID = arithexpr ;
boolexpr  -> arithexpr boolop arithexpr
boolop -> <
boolop -> >
boolop -> <=
boolop -> >=
boolop -> ==
arithexpr -> multexpr arithexprprime
arithexprprime -> + multexpr arithexprprime
arithexprprime -> - multexpr arithexprprime
arithexprprime -> E
multexpr -> simpleexpr  multexprprime
multexprprime -> * simpleexpr multexprprime
multexprprime -> / simpleexpr multexprprime
multexprprime -> E
simpleexpr -> ID
simpleexpr -> NUM
simpleexpr -> ( arithexpr ))");
	llp.execute(prog);
	/********* End *********/

}