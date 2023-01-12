/*****************************************************************//**
 * \file   LRparser.h
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
#include <deque>
#include <set>
using namespace std;
/* 不要修改这个标准输入函数 */
void read_prog(string& prog)
{
	char c;
	while (scanf("%c", &c) != EOF) {
		if (c == '#')
			break;
		prog += c;
	}
}
/* 你可以添加其他函数 */

/**
 * 将原始输入分词产生token，主要使用单个词的string和行号两个属性.
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
///SLR parsing table表项对应的枚举类型
enum{SHIFT,REDUCE,GOTO,ACC};
struct entry {
	int type;
	int num;
	entry(){}
	entry(int t, int n) { type = t; num = n; }
};

vector<token> split_word(string input);

///error的枚举类型
enum{UNEXPECTED_SYMBOL,EARLY_EOF,REPEAT_ENTRY};
struct error {
	int type;
	int line_num;

	error() {}
	error(int t, int ln) {
		type = t; line_num = ln;
	}
};
/**
 * 错误处理类，用来记录和打印错误信息.
 */
class ErrorHandler {
private:
	vector<error> errors;
public:
	bool report();
	void add(int error, int ln);
};

void ErrorHandler::add(int etype, int ln) {
	errors.push_back(error(etype, ln));
}

bool ErrorHandler::report() {
	if (errors.empty()) {
		return false;
	}
	else {
		for (auto e : errors) {
			cout << "语法错误，第" << e.line_num << "行，";
			switch (e.type) {
			case UNEXPECTED_SYMBOL:
				cout << "缺少\";\"" << endl;
				break;
			case EARLY_EOF:
				cout << "这里不应该是EOF,你的代码不完整,请检查" << endl;
				break;
			default:
				cout << "unkown error" << endl;
			}
		}
		return true;
	}
}

struct item {
	int pid;
	int idx;
	item(){}
	item(int p, int i) { pid = p; idx = i; }
};

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
	vector< vector<entry>> slrtable;
	map<string, int> sid;
	vector< vector<item>> canonical_collection;
	Generator();
	Generator(string raw_rules);
	void generate_dic();
	void generate_first();
	void generate_follow();
	void generate_LLtable();
	vector<item> closure(vector<item> init_state);
	vector<item> get_goto(vector<item> from_state,string s);
	int state_id(vector<item> target);
	bool repeat_item(vector<item> it, int i);
	void generate_canonical_collection();
	void generate_SLRtable();
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
	int curline = -1, pos = 0;
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
			if (!nonterm.count(p) && p != "E")
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
	vector<entry> slrrow;
	for (int i = 0; i < term.size() + nonterm.size() + 1; i++) {
		row.push_back(-1);
		slrrow.push_back(entry(-1,-1));
	}
	for (int i = 0; i < nonterm.size() + 1; i++)
		table.push_back(row);
	for (int i = 0; i < 200; i++)
		slrtable.push_back(slrrow);
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
				if (term.count(ps[j]) || !nullable[ps[j]])
					go_nullable = false;
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
			} while (nullable[ps[j]] || ps[j] == "E");
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
		for (auto ps : productions) {
			pid++;
			bool end_null = true;
			for (int j = ps.size() - 1; j >= 2; j--) {
				if (nonterm.count(ps[j - 1])) {

					if (term.count(ps[j])) {
						follow[sid[ps[j - 1]]].insert(ps[j]);
						if (nullable[ps[j - 1]])
							table[sid[ps[j - 1]]][sid[ps[j]]] = null_pid[ps[j - 1]];
					}
					else {
						for (auto f : first[sid[ps[j]]]) {
							follow[sid[ps[j - 1]]].insert(f);
							if (nullable[ps[j - 1]])
								table[sid[ps[j - 1]]][sid[f]] = null_pid[ps[j - 1]];
						}
					}
				}
				if (nonterm.count(ps[j]) && end_null) {
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
/**
 * 检查目标item是否已经存在于item集中，
 * its是item集，将其中每一个item与第 i 条产生式，点在头上的item比较.
 * 
 * \param its
 * \param i
 * \return 
 */
bool Generator::repeat_item(vector<item> its, int i) {
	for (auto it : its) {
		if (it.pid == i && it.idx == 0)
			return true;
	}
	return false;
}
/**
 * 检查状态是否已经存在于状态集中，
 * 如果存在返回状态id，否则返回-1.
 * 
 * \param target
 * \return 
 */
int Generator::state_id(vector<item> target) {
	for (int i = 0; i < canonical_collection.size(); i++) {
		if (canonical_collection[i].size() != target.size())
			continue;
		int same_item = 0;
		vector<item> temp = canonical_collection[i];
		for (int j = 0; j < temp.size(); j++) {
			int cnt = 0;
			for (auto t : target) {
				if (temp[j].idx == t.idx&&temp[j].pid == t.pid)
					break;
				cnt++;
			}
			if (cnt < target.size())
				same_item++;
		}
		if (same_item == temp.size()){
			return i;
		}
	}
	return -1;
}

/**
 * 根据状态中的初始item集计算闭包.
 * 
 * \param init_state
 * \return 
 */
vector<item> Generator::closure(vector<item> init_state) {
	deque<item> item_stack;
	for (auto it : init_state)
		item_stack.push_front(it);

	while (!item_stack.empty()) {
		item itemhead = item_stack.front();
		item_stack.pop_front();
		vector<string> production = productions[itemhead.pid];
		if (itemhead.idx < production.size()-1) {
			string temp = production[itemhead.idx + 1];
			if (nonterm.count(temp)) {
				for (int i = 0; i < productions.size(); i++) {
					if (productions[i][0]==temp && !repeat_item(init_state,i)) {
						init_state.push_back(item(i,0));
						item_stack.push_back(item(i,0));
					}
				}
			}
		}
	}
	return init_state;
}

/**
 * 计算根据状态和接受的终结/非终结符计算goto状态.
 * 
 * \param from_state
 * \param s
 * \return 
 */
vector<item> Generator::get_goto(vector<item> from_state, string s) {
	vector<item> temp;
	///先把所有点往后挪一位
	for (int i = 0; i < from_state.size(); i++) {
		int pid = from_state[i].pid;
		int idx = from_state[i].idx;
		if (idx+1<productions[pid].size()&&productions[pid][idx + 1].compare(s) == 0)
			temp.push_back(item(pid,idx+1));
	}
	///计算闭包
	return closure(temp);
}
/**
 * 计算所有的状态集，
 * 用栈，出栈已有的状态，计算goto状态压入状态集，
 * 重复这个过程直到栈空.
 * 
 */
void Generator::generate_canonical_collection() {
	vector<item> first_state;
	first_state.push_back(item(0,0));
	first_state = closure(first_state);

	canonical_collection.push_back(first_state);

	deque< vector<item>> state_stack;
	deque<int> id_stack;
	state_stack.push_front(first_state);
	id_stack.push_front(0);
	while (!state_stack.empty()) {
		vector<item> statehead = state_stack.front();
		state_stack.pop_front();

		int idhead = id_stack.front();
		id_stack.pop_front();

		for (auto nt:nonterm) {
			vector<item> temp = get_goto(statehead, nt);
			if (temp.size()!=0) {
				int id = state_id(temp);
				if (id == -1) {
					id = canonical_collection.size();
					state_stack.push_front(temp);
					id_stack.push_front(id);
					canonical_collection.push_back(temp);
				}
				slrtable[idhead][sid[nt]] = entry(GOTO,id);
			}
		}
		
		for (auto t:term) {
			if (t == "E")
				continue;
			vector<item> temp = get_goto(statehead, t);
			if (temp.size() != 0) {
				int id = state_id(temp);
				if (id == -1) {
					id = canonical_collection.size();
					state_stack.push_front(temp);
					id_stack.push_front(id);
					canonical_collection.push_back(temp);
				}
				slrtable[idhead][sid[t]] = entry(SHIFT, id);
			}
		}
		///处理reduce表项
		for (auto it : statehead) {
			if (it.idx == productions[it.pid].size() - 1||
				(productions[it.pid].size()==2&&productions[it.pid][1]=="E")) {
				for (auto f : follow[sid[productions[it.pid][0]]])
					slrtable[idhead][sid[f]] = entry(REDUCE, it.pid);
			}
		}
	}
	slrtable[0][sid[productions[0][1]]] = entry(ACC, -1);
}

void Generator::generate_SLRtable() {
	generate_dic();
	generate_first();
	generate_follow();
	generate_canonical_collection();
}

/**
 * 分词函数，
 * 将输入的字符串分割成带有行号信息的token.
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
			if(line != "")
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

class  LRparser {
private:
	vector<token> tokens;
	deque<token> input;
	deque<token> work;
	deque<int> state;
	vector<entry> op;
	deque<string> output;
	ErrorHandler e;
	Generator g;
public:
	void init_table(string raw_input);
	vector<entry> execute(string input);
	void show();
};

void LRparser::init_table(string raw_input) {
	g = Generator(raw_input);
	g.generate_dic();
	g.generate_first();
	g.generate_follow();
	g.generate_canonical_collection();
}
/**
 * 语法处理函数，
 * 接收对应代码，先分词，
 * 用deque模拟栈，进行Bottom-up的语法处理.
 * 
 * \param input_string
 * \return 
 */
vector<entry> LRparser::execute(string input_string) {
	tokens = split_word(input_string);
	for (int i = tokens.size() - 1; i >= 0; i--)
		input.push_front(tokens[i]);
	input.push_back(token("$", -1, -1, -1));
	state.push_front(0);

	token workhead; token inputhead; int statehead; entry curop; int curline=1;

	while (!input.empty()) {
		statehead = state.front();
		inputhead = input.front();
		//cout << "line " << curline << " : " << statehead << ' ' << inputhead.str << ' ';
		curop = g.slrtable[statehead][g.sid[inputhead.str]];
		//cout<<curop.type<<' '<<curop.num<<endl;
		if (curop.type == ACC)
			break;
		if (curop.type == SHIFT) {
			work.push_front(inputhead);
			input.pop_front();
			state.push_front(curop.num);
		}
		if (curop.type == REDUCE) {
			op.push_back(curop);
			for (int i = 1; i < g.productions[curop.num].size(); i++) {
				if (g.productions[curop.num][i] == "E")
					continue;
				work.pop_front();
				state.pop_front();
			}
			work.push_front(token(g.productions[curop.num][0], -1, -1, inputhead.line_num));
			entry temp = g.slrtable[state.front()][g.sid[g.productions[curop.num][0]]];
			//cout << state.front() << ' ' << g.productions[curop.num][0] << ' ' << temp.type << ' ' << temp.num << endl;
			state.push_front(temp.num);
			if (temp.type == ACC)
				break;
		}
		if (curop.type == -1) {
			input.push_front(token(";", -1, -1, curline));
			e.add(UNEXPECTED_SYMBOL, curline);
		}
		curline = inputhead.line_num;
	}

	return op;
}
/**
 * 展示语法处理的结果.
 * 
 */
void LRparser::show() {
	e.report();
	output.push_back("program");
	string s = "";
	cout << "program ";
	for (int i = op.size() - 1; i >= 0; i--) {
		//cout<<op[i]<<endl;
		output.pop_back();
		int num = op[i].num;
		for (int j = 1; j < g.productions[num].size(); j++)
			output.push_back(g.productions[num][j]);
		while (!output.empty()) {
			if (!g.term.count(output.back()))
				break;
			string s_add="";
			if (output.back() != "E")
				s_add = output.back() + " ";
			s = s_add + s;
			output.pop_back();
		}
		cout << "=> \n";
		for (int j = 0; j < output.size(); j++)
			cout << output[j] << ' ';
		cout << s;
	}
}

void Analysis()
{
	string prog;
	read_prog(prog);
	/* 骚年们 请开始你们的表演 */
	/********* Begin *********/
	LRparser lrp = LRparser();
	lrp.init_table(R"(programprime -> program
program -> compoundstmt
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
boolexpr -> arithexpr boolop arithexpr
boolop -> <
boolop -> >
boolop -> <=
boolop -> >=
boolop -> ==
arithexpr -> multexpr arithexprprime
arithexprprime -> + multexpr arithexprprime
arithexprprime -> - multexpr arithexprprime
arithexprprime -> E
multexpr -> simpleexpr multexprprime
multexprprime -> * simpleexpr multexprprime
multexprprime -> / simpleexpr multexprprime
multexprprime -> E
simpleexpr -> ID
simpleexpr -> NUM
simpleexpr -> ( arithexpr ))");
	lrp.execute(prog);
	lrp.show();
	/********* End *********/

}