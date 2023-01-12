// C���Դʷ�������
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
/* ��Ҫ�޸������׼���뺯�� */
void read_prog(string& prog)
{
	char c;
	while (scanf("%c", &c) != EOF) {
		if (c == '#')
			break;
		prog += c;
	}
}
/* ���������������� */

///error��ö������
enum { DIVIDE_BY_ZERO, REAL_TO_INT, INT_TO_NUM, DELIMETRE, UNDECLARED_ID, UNDECLARED_ID_ASSIGN, UNKONW };
///ID��������token��ö������
enum { REAL, INT,ID };
///�����ö������
enum { EMPTY,ASSIGN, BOOL, ARITH, IF, DECLARE ,ARITHPRIME,INSTANT,IDVALUE,BOOLOP};
///LRparser�����ö������
enum { SHIFT, REDUCE, GOTO, ACC };

struct token {
	string str;
	double value;
	string name;
	int type;
	int line_num;

	token() {}
	token(string s, double v, int t, int ln) {
		str = s; value = v; type = t; line_num = ln;
	}
	token(string s, string nn, int t, int ln) {
		str = s; name = nn; type = t; line_num = ln;
	}
};

struct entry {
	int type;
	int num;
	entry() {}
	entry(int t, int n) { type = t; num = n; }
};

struct expr {
	entry op;
	vector<token> tokens;

	expr(){}
	expr(entry o, vector<token> ts) {
		op = o; tokens = ts;
	}
};

struct error {
	int type;
	int line_num;

	error() {}
	error(int t, int ln) {
		type = t; line_num = ln;
	}
};

struct item {
	int pid;
	int idx;
	item() {}
	item(int p, int i) { pid = p; idx = i; }
};

vector<token> split_word(string s);
/**
 * �������࣬������¼�ʹ�ӡ������Ϣ.
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
			cout << "error message:line " << e.line_num << ",";
			switch (e.type) {
			case DIVIDE_BY_ZERO:
				cout << "division by zero" << endl;
				break;
			case REAL_TO_INT:
				cout << "realnum can not be translated into int type"<<endl;
				break;
			case UNDECLARED_ID:
				cout << "cannot get the value of undeclared id" << endl;
				break;
			case UNDECLARED_ID_ASSIGN:
				cout << "cannot assign value to undeclared id" << endl;
				break;
			default:
				cout << "unkown error" << endl;
			}
		}
		return true;
	}
}

/**
 * Generator�࣬���ܹ淶���ķ�����"|"ת���ɶ�������ʽ�����зָE��ʾ�գ���
 * �Զ�����LL(1) parsing table.
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
	vector<item> get_goto(vector<item> from_state, string s);
	int state_id(vector<item> target);
	bool repeat_item(vector<item> it, int i);
	void generate_canonical_collection();
	void generate_SLRtable();
};

Generator::Generator() {}
/**
 * ��ԭʼ��rule���зָ����
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
 * �����ֵ䣬��¼�ս���ͷ��ս����
 * ͬʱ��ֱ�Ӳ����յķ��ս����Ϊnullable��
 * ����¼�ղ���ʽ��Ӧ�Ĳ���ʽid.
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
		slrrow.push_back(entry(-1, -1));
	}
	for (int i = 0; i < nonterm.size() + 1; i++)
		table.push_back(row);
	for (int i = 0; i < 200; i++)
		slrtable.push_back(slrrow);
}
/**
 * ��ȷ�������ս���Ƿ�Ϊnullable��
 * Ȼ������first��������first�����һ����LL(1) table.
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
 * ����follow������һ�����ȫ����LL(1) table.
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
 * ���Ŀ��item�Ƿ��Ѿ�������item���У�
 * its��item����������ÿһ��item��� i ������ʽ������ͷ�ϵ�item�Ƚ�.
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
 * ���״̬�Ƿ��Ѿ�������״̬���У�
 * ������ڷ���״̬id�����򷵻�-1.
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
		if (same_item == temp.size()) {
			return i;
		}
	}
	return -1;
}

/**
 * ����״̬�еĳ�ʼitem������հ�.
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
		if (itemhead.idx < production.size() - 1) {
			string temp = production[itemhead.idx + 1];
			if (nonterm.count(temp)) {
				for (int i = 0; i < productions.size(); i++) {
					if (productions[i][0] == temp && !repeat_item(init_state, i)) {
						init_state.push_back(item(i, 0));
						item_stack.push_back(item(i, 0));
					}
				}
			}
		}
	}
	return init_state;
}

/**
 * �������״̬�ͽ��ܵ��ս�/���ս������goto״̬.
 *
 * \param from_state
 * \param s
 * \return
 */
vector<item> Generator::get_goto(vector<item> from_state, string s) {
	vector<item> temp;
	///�Ȱ����е�����Ųһλ
	for (int i = 0; i < from_state.size(); i++) {
		int pid = from_state[i].pid;
		int idx = from_state[i].idx;
		if (idx + 1 < productions[pid].size() && productions[pid][idx + 1].compare(s) == 0)
			temp.push_back(item(pid, idx + 1));
	}
	///����հ�
	return closure(temp);
}
/**
 * �������е�״̬����
 * ��ջ����ջ���е�״̬������goto״̬ѹ��״̬����
 * �ظ��������ֱ��ջ��.
 *
 */
void Generator::generate_canonical_collection() {
	vector<item> first_state;
	first_state.push_back(item(0, 0));
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

		for (auto nt : nonterm) {
			vector<item> temp = get_goto(statehead, nt);
			if (temp.size() != 0) {
				int id = state_id(temp);
				if (id == -1) {
					id = canonical_collection.size();
					state_stack.push_front(temp);
					id_stack.push_front(id);
					canonical_collection.push_back(temp);
				}
				slrtable[idhead][sid[nt]] = entry(GOTO, id);
			}
		}

		for (auto t : term) {
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
		///������������������ô�͸���follow������reduce entry
		for (auto it : statehead) {
			if (it.idx == productions[it.pid].size() - 1 ||
				(productions[it.pid].size() == 2 && productions[it.pid][1] == "E")) {
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

class  LRparser {
private:
	vector<token> tokens;
	deque<token> input;
	deque<token> work;
	deque<int> state;
	vector<entry> op;
	deque<string> output;
	map<int, int> rtype;
	ErrorHandler e;
	vector<expr> exprs;
	map<string, double> id_value;
	map<string, int> id_type;
	Generator g;
public:
	void init_table(string raw_input);
	vector<entry> execute(string input);
	void show();
	vector<token> split_word(string s);
	token str_to_token(string s,int ln);
	void translate();
};

void LRparser::init_table(string raw_input) {
	g = Generator(raw_input);
	g.generate_dic();
	g.generate_first();
	g.generate_follow();
	g.generate_canonical_collection();

	rtype[4] = DECLARE; rtype[5] = DECLARE; rtype[12] = IF; rtype[13] = ASSIGN; rtype[14] = BOOL; rtype[20] = ARITH;
	rtype[21] = ARITHPRIME; rtype[22] = ARITHPRIME; rtype[24] = ARITH; rtype[25] = ARITHPRIME; rtype[26] = ARITHPRIME;
	rtype[28] = IDVALUE; rtype[29] = INSTANT; rtype[30] = INSTANT; rtype[31] = ARITH;
	rtype[15] = BOOLOP; rtype[16] = BOOLOP; rtype[17] = BOOLOP; rtype[18] = BOOLOP; rtype[19] = BOOLOP;
}
/**
 * ��������ת���ɴ���type��value���Լ��к���Ϣ��token�ĺ�����
 * ��Ҫ�����ǻ�ȡ�������͵�value���Լ�����ʶ��ת��Ϊ����name��ID token.
 * 
 * \param word
 * \param ln
 * \return 
 */
token LRparser::str_to_token(string word, int ln) {
	if (g.term.count(word))
		return token(word, -1, -1, ln);
	else {
		if (word[0] >= '0'&&word[0] <= '9') {
			vector<double> res;
			int dot_pos = word.length();
			for (int i = 0; i < word.length(); i++) {
				if (word[i] == '.')
					dot_pos = i;
			}
			double temp = 0;
			double mult = 1;
			for (int i = dot_pos - 1; i >= 0; i--) {
				temp += (word[i] - '0')*mult;
				mult *= 10;
			}
			res.push_back(temp);
			temp = 0;
			mult = 0.1;
			if (dot_pos != word.length()) {
				for (int i = dot_pos + 1; i < word.length(); i++) {
					temp += (word[i] - '0')*mult;
					mult /= 10;
				}
				res.push_back(temp);
			}
			int ntype = (res.size() == 1) ? INT : REAL;
			if (ntype == REAL)
				return token("REALNUM", res[0] + res[1], REAL, ln);
			else
				return token("INTNUM", res[0], INT, ln);
		}
		else
			return token("ID", word, ID, ln);
	}
}
/**
 * �ִʺ�����
 * ��������ַ����ָ�ɴ����к���Ϣ��token,
 * �������ж�type��value���Ա�ʶ��ת��Ϊ��name��ID token.
 * 
 * \param s
 * \return 
 */
vector<token> LRparser::split_word(string s) {
	vector<string> split_temp;
	vector<token> split_res;
	split_res.clear();
	string line = "";
	string word = "";
	for (auto c : s) {
		if (c == '\n') {
			if(line!="")
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
				if (word != "") {
					split_res.push_back(str_to_token(word, i + 1));
				}
				word = "";
			}
			else
				word += c;
		}
		if (word != "")
			split_res.push_back(str_to_token(word, i + 1));
	}
	return split_res;
}
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
 * �﷨��������
 * ���ն�Ӧ���룬�ȷִʣ�
 * ��dequeģ��ջ������Bottom-up���﷨����
 * �洢ÿһ�����еĲ�������Ҫ��reduction�������Թ��﷨�Ƶ�����ʹ��.
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

	token workhead; token inputhead; int statehead; entry curop; int curline = 1;

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
			vector<token> ts;
			op.push_back(curop);
			ts.push_back(token("#",-1,-1,-1));
			for (int i = 1; i < g.productions[curop.num].size(); i++) {
				if (g.productions[curop.num][i] == "E")
					continue;
				ts.push_back(work.front());
				work.pop_front();
				state.pop_front();
			}
			exprs.push_back(expr(curop, ts));
			work.push_front(token(g.productions[curop.num][0], -1, -1, inputhead.line_num));
			entry temp = g.slrtable[state.front()][g.sid[g.productions[curop.num][0]]];
			//cout << state.front() << ' ' << g.productions[curop.num][0] << ' ' << temp.type << ' ' << temp.num << endl;
			state.push_front(temp.num);
			if (temp.type == ACC)
				break;
		}
		if (curop.type == -1) {
			if (inputhead.str == "REALNUM") {
				inputhead.str = "INTNUM";
				input.pop_front(); input.push_front(inputhead);
				e.add(REAL_TO_INT, curline);
			}
		}
		curline = inputhead.line_num;
	}

	return op;
}
/**
 * չʾ�﷨����Ľ��.
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
			string s_add = "";
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
/**
 * �﷨�Ƶ����룬����Bottom-up���﷨�Ƶ����룬
 * �����ض���reduction������������ִ���ض������.
 * 
 */
void LRparser::translate() {
	vector<string> ids;
	double curvalue = 0,opvalue=0;
	bool holding = false;
	int if_true = 0;
	string compare_op;
	for (int i = 0; i < exprs.size();i++) {
		//cout << exprs[i].op<<' '<<curvalue<<' '<<opvalue<<' '<<holding<< endl;
		vector<token> t = exprs[i].tokens;
		/*for (int i=t.size()-1;i>=0;i--)
			cout << t[i].str << ' ';
		cout << endl;*/
		///������䣬��ids���������м����ʶ����name�ͣ����趨��ʼֵ��type
		if (rtype[exprs[i].op.num] == DECLARE) {
			int type1;
			if (t[t.size() - 1].str == "int")
				type1 = INT;
			else
				type1 = REAL;
			ids.push_back(t[t.size() - 2].name);
			id_type.insert(pair<string, int>(t[t.size() - 2].name, type1));
			id_value.insert(pair<string, double>(t[t.size() - 2].name, t[t.size() - 4].value));
		}
		///��������������߱�ʶ����Ӧ��ֵ����ֵ�����ǰ�ﾳ��
		else if (rtype[exprs[i].op.num] == INSTANT) {
			if (!holding)
				curvalue = t[t.size() - 1].value;
			else {
				opvalue = t[t.size() - 1].value;
			}
			holding = true;
		}
		else if (rtype[exprs[i].op.num] == IDVALUE) {
			if (id_value.find(t[t.size() - 1].name) == id_value.end())
				e.add(UNDECLARED_ID, t[t.size() - 1].line_num);
			if (!holding)
				curvalue = id_value[t[t.size() - 1].name];
			else {
				opvalue = id_value[t[t.size() - 1].name];
			}
			holding = true;
		}
		///����Ǹ�ֵ��䣬���ﾳ�е�ֵ����Ŀ���ʶ������������ﾳ�е�ֵ
		else if (rtype[exprs[i].op.num] == ASSIGN) {
			if (id_value.find(t[t.size() - 1].name) == id_value.end())
				e.add(UNDECLARED_ID_ASSIGN, t[t.size() - 1].line_num);
			if (if_true != -1) {
				id_value[t[t.size() - 1].name] = curvalue;
				holding = false;
			}
			if (if_true == 1)
				if_true = -1;
			else if (if_true == -1)
				if_true = 0;
			curvalue = 0; opvalue = 0; holding = false;
		}
		///���������������ﾳ�е�ֵ���д����Ҵ���ﾳ��
		else if (rtype[exprs[i].op.num] == ARITHPRIME) {
			switch (t[3].str[0]) {
			case '+':
				curvalue += opvalue;
				break;
			case '-':
				curvalue -= opvalue;
				break;
			case '*':
				curvalue *= opvalue;
				break;
			case '/':
				if (opvalue == 0)
					e.add(DIVIDE_BY_ZERO, t[3].line_num);
				curvalue /= opvalue;
				break;
			}

		}
		else if (rtype[exprs[i].op.num] == BOOLOP)
			compare_op=t[1].str;
		else if (rtype[exprs[i].op.num] == BOOL) {
			if (compare_op == "<=")
				if_true = (curvalue <= opvalue) ? 1 : -1;
			else if (compare_op == ">=")
				if_true = (curvalue >= opvalue) ? 1 : -1;
			else if (compare_op == ">")
				if_true = (curvalue > opvalue) ? 1 : -1;
			else if (compare_op == "<")
				if_true = (curvalue < opvalue) ? 1 : -1;
			else if (compare_op == "==")
				if_true = (curvalue == opvalue) ? 1 : -1;
			curvalue = 0; opvalue = 0;
			holding = false;
		}
	}
	///��ӡ�����������﷨�Ƶ�����Ľ��
	if (!e.report()) {
		for (auto id : ids) {
			cout << id << ": " << id_value[id] << endl;
		}
	}
}

void Analysis()
{
	string prog;
	read_prog(prog);
	/* ɧ���� �뿪ʼ���ǵı��� */
	/********* Begin *********/
	LRparser lrp = LRparser();
	lrp.init_table(R"(program' -> program
program -> decls compoundstmt
decls -> decl ; decls
decls -> E
decl -> int ID = INTNUM
decl -> real ID = REALNUM
stmt -> ifstmt
stmt -> assgstmt
stmt -> compoundstmt
compoundstmt -> { stmts }
stmts -> stmt stmts
stmts -> E
ifstmt -> if ( boolexpr ) then stmt else stmt
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
simpleexpr -> INTNUM
simpleexpr -> REALNUM
simpleexpr -> ( arithexpr ))");
	lrp.execute(prog);
	lrp.translate();
	/********* End *********/

}
