#include <stdlib.h>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

const string default_uservices_path = "../..";
const string userver_path = ".";

string add_path(const string& pref, const string& suf) {
  if (pref.empty()) return suf;
  if (suf.empty()) return pref;
  auto res = pref;
  if (res.back() != '/' && suf.front() != '/') res += '/';
  res += suf;
  return res;
}

const string client_filename =
    add_path(userver_path, "redis/include/storages/redis/client.hpp");
const string client_impl_hpp_filename =
    add_path(userver_path, "redis/src/storages/redis/client_impl.hpp");
const string client_impl_cpp_filename =
    add_path(userver_path, "redis/src/storages/redis/client_impl.cpp");
const string request_filename =
    add_path(userver_path, "redis/include/storages/redis/request.hpp");
const string transaction_hpp_filename =
    add_path(userver_path, "redis/include/storages/redis/transaction.hpp");
const string transaction_impl_hpp_filename =
    add_path(userver_path, "redis/src/storages/redis/transaction_impl.hpp");
const string transaction_impl_cpp_filename =
    add_path(userver_path, "redis/src/storages/redis/transaction_impl.cpp");
const string mock_client_base_test_hpp_filename = add_path(
    userver_path, "redis/testing/include/storages/redis/mock_client_base.hpp");
const string mock_client_base_test_cpp_filename = add_path(
    userver_path, "redis/testing/src/storages/redis/mock_client_base.cpp");
const string mock_transaction_test_hpp_filename = add_path(
    userver_path, "redis/testing/include/storages/redis/mock_transaction.hpp");
const string mock_transaction_test_cpp_filename = add_path(
    userver_path, "redis/testing/src/storages/redis/mock_transaction.cpp");
const string mock_transaction_impl_base_test_hpp_filename = add_path(
    userver_path,
    "redis/testing/include/storages/redis/mock_transaction_impl_base.hpp");
const string mock_transaction_impl_base_test_cpp_filename =
    add_path(userver_path,
             "redis/testing/src/storages/redis/mock_transaction_impl_base.cpp");

const string tmp_filename = "/tmp/add_redis_command_tmp.txt";

const string redis_commands_str = "redis commands:";
const string redis_commands_end_str = "end of redis commands";
const string delim = "\n\n";
const string equal_zero = ") = 0;";
const string not_equal_zero = ");";
const string override_ending = ") override;";
const string big_a_z = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

char buf[2 << 20];

string read_file(const string& filename) {
  ifstream f(filename);
  f.read(buf, sizeof(buf));
  size_t read_bytes = f.gcount();
  assert(read_bytes < sizeof(buf));
  return string(buf, buf + read_bytes);
}

struct param_t {
  string type;
  string name;
};

string uservices_path;
string command_cc;
string command;
bool master;
vector<param_t> params;
string request_tmpl_param;  // 'cpp_type_to_parse[,
                            // reply_cpp_type=cpp_type_to_parse]'
bool dry_run = false;

void fail(const string& msg = {}) {
  cerr << "FAIL " << msg << endl;
  abort();
}

void write_file(const string& filename, const std::string& data) {
  if (dry_run) {
    string lastname = filename;
    auto pos = lastname.rfind('/');
    if (pos != string::npos) lastname = lastname.substr(pos + 1);
    string new_file_tmp = "/tmp/" + lastname;
    ofstream f(new_file_tmp);
    f.write(data.c_str(), data.size());
    int call_res = system(
        ("diff -u " + filename + " " + new_file_tmp + " >" + tmp_filename)
            .c_str());
    cerr << "call_res=" << call_res << endl;
    auto diff = read_file(tmp_filename);
    if (diff.empty()) return;
    if (diff.substr(0, 3) != "---") fail("can't parse diff.\n" + diff);
    auto nl = diff.find('\n');
    if (nl == string::npos) fail("can't parse diff.\n" + diff);
    auto pref_len = add_path(uservices_path, userver_path).size() + 1;
    auto fn = diff.substr(4, diff.find('\t', 4) - 4);
    auto dir = fn.substr(0, fn.rfind('/')).substr(pref_len);
    auto ppp = diff.find("+++", nl);
    size_t start_fn = 4 + pref_len;  // "--- .././"
    diff = diff.substr(0, 4) + diff.substr(start_fn, ppp + 4 - start_fn) + dir +
           diff.substr(ppp + 8);
    cout << diff << endl;
    return;
  }
  ofstream f(filename);
  f.write(data.c_str(), data.size());
}

void get_commands(const string& file, vector<size_t>& command_ends,
                  vector<size_t>& command_names_pos,
                  vector<string>& command_names) {
  auto redis_commands_pos = file.find(redis_commands_str);
  if (redis_commands_pos == string::npos) fail("start not found");
  auto stop = file.find(redis_commands_end_str, redis_commands_pos);
  if (stop == string::npos) fail("stop not found");

  command_ends.emplace_back(redis_commands_pos + redis_commands_str.size());

  for (;;) {
    auto brpos = file.find_first_of(big_a_z, command_ends.back());
    brpos = file.find("(", brpos);
    if (brpos == string::npos || brpos >= stop) break;
    auto cmd_start = file.find_last_of(" :", brpos);
    if (cmd_start == string::npos) fail("cmd_start not found");
    ++cmd_start;
    auto pos = file.find(delim, brpos);
    if (pos == string::npos || pos >= stop) break;
    command_names_pos.emplace_back(cmd_start);
    command_names.emplace_back(file.substr(cmd_start, brpos - cmd_start));
    command_ends.emplace_back(pos);
  }

  assert(command_names_pos.size() == command_names.size());
  assert(command_names_pos.size() + 1 == command_ends.size());
  for (size_t i = 1; i < command_names.size(); i++) {
    if (command_names[i - 1] > command_names[i]) {
      cerr << command_names[i - 1] << " > " << command_names[i] << " i=" << i
           << endl;
    }
    assert(command_names[i - 1] <= command_names[i]);
  }

  cerr << "current_commands_count=" << command_names.size() << endl;
  for (const auto& command : command_names) {
    cerr << command << endl;
  }
}

void process_file(const string& client_filename, const string& decl_command) {
  string client_filename_full = add_path(uservices_path, client_filename);
  string client_file = read_file(client_filename_full);
  //	cerr << client_file << endl;

  vector<size_t> command_ends;
  vector<size_t> command_names_pos;
  vector<string> command_names;

  get_commands(client_file, command_ends, command_names_pos, command_names);

  size_t idx =
      upper_bound(command_names.begin(), command_names.end(), command_cc) -
      command_names.begin();

  cerr << decl_command << endl;
  client_file.insert(command_ends[idx], delim + decl_command);

  write_file(client_filename_full, client_file);
}

string gen_first_line_cpp(const string& class_name, bool add_command_control,
                          bool comment_param_names) {
  string decl_command =
      "Request" + command_cc + ' ' + class_name + "::" + command_cc + '(';
  bool first = true;
  for (const auto& param : params) {
    if (first)
      first = false;
    else
      decl_command += ", ";
    decl_command += param.type + ' ';
    if (comment_param_names) decl_command += "/*";
    decl_command += param.name;
    if (comment_param_names) decl_command += "*/";
  }
  if (add_command_control) {
    if (first)
      first = false;
    else
      decl_command += ", ";
    decl_command += "const CommandControl& command_control";
  }
  decl_command += ") {\n";
  return decl_command;
}

void process_client_file() {
  cerr << "--- " << __func__ << " ---" << endl;

  string decl_command =
      "  virtual Request" + command_cc + ' ' + command_cc + '(';
  for (const auto& param : params) {
    decl_command += param.type + ' ' + param.name + ", ";
  }
  decl_command += "const CommandControl& command_control" + equal_zero;

  process_file(client_filename, decl_command);
}

void process_client_impl_hpp_file() {
  cerr << "--- " << __func__ << " ---" << endl;

  string decl_command = "  Request" + command_cc + ' ' + command_cc + '(';
  for (const auto& param : params) {
    decl_command += param.type + ' ' + param.name + ", ";
  }
  decl_command += "const CommandControl& command_control" + override_ending;

  process_file(client_impl_hpp_filename, decl_command);
}

bool need_move(const std::string& type) {
  return type.find("const") == string::npos &&
         (type.find("std::string") != string::npos ||
          type.find("std::vector") != string::npos ||
          type.find("GeoaddArg") != string::npos);
}

void add_dummy_request(const string& param_name, string& decl_command) {
  decl_command += "  if (" + param_name + ".empty())\n";
  decl_command += "    return CreateDummyRequest<Request" + command_cc + ">(\n";
  decl_command +=
      "        std::make_shared<::redis::Reply>(\"" + command + "\", ";
  if (request_tmpl_param.find("std::vector") == 0)
    decl_command += "::redis::ReplyData::Array{}";
  else if (request_tmpl_param.find("Status") == 0) {
    string status = request_tmpl_param.substr(6);
    size_t pos = 0;
    while (pos < status.size() && isalpha(status[pos])) ++pos;
    status.resize(pos);
    for (char& ch : status) {
      ch = toupper(ch);
    }
    decl_command += "::redis::ReplyData::CreateStatus(\"" + status + "\")";
  } else if (request_tmpl_param == "size_t") {
    decl_command += "0";
  } else {
    cerr << "not implemented for request_tmpl_param=" << request_tmpl_param
         << endl;
    abort();
  }
  decl_command += "));\n";
}

bool is_shard_found() {
  bool shard_found = false;
  for (const auto& param : params) {
    if (param.name == "shard") {
      shard_found = true;
      break;
    }
  }
  return shard_found;
}

void process_client_impl_cpp_file() {
  cerr << "--- " << __func__ << " ---" << endl;

  static const string decl_ending = "\n}";

  string decl_command = gen_first_line_cpp("ClientImpl", true, false);
  bool shard_found = is_shard_found();
  if (shard_found) {
    decl_command += "  CheckShard(shard, command_control);\n";
  } else {
    if (params[0].name == "key") {
      for (const auto& param : params) {
        if (param.type.find("std::vector") == 0 &&
            (param.name == "members" || param.name == "fields" ||
             param.name == "scored_members")) {
          add_dummy_request(param.name, decl_command);
        }
      }
      decl_command += "  auto shard = ShardByKey(key, command_control);\n";
    } else if (params[0].name == "keys") {
      add_dummy_request(params[0].name, decl_command);
      decl_command +=
          "  auto shard = ShardByKey(keys.at(0), command_control);\n";
    } else if (params[0].name == "key_values") {
      add_dummy_request(params[0].name, decl_command);
      decl_command +=
          "  auto shard = ShardByKey(key_values.at(0).first, "
          "command_control);\n";
    } else {
      cerr << "can't determine shard by params" << endl;
      abort();
    }
  }
  for (const auto& param : params) {
    if (param.name == "shard") continue;
    if (param.name == "keys") continue;
    if (param.name == "key_values") continue;
    if (param.type.find("vector") == string::npos) continue;
    if (param.name == "members") continue;
    if (param.name == "fields") continue;
    if (param.name == "scored_members") continue;
    decl_command += "  CheckNonEmpty(" + param.name + ");\n";
  }
  decl_command += "  return CreateRequest<Request" + command_cc +
                  ">(MakeRequest(CmdArgs{\"" + command + "\"";
  for (const auto& param : params) {
    if (param.name == "shard") continue;
    decl_command += ", ";
    if (need_move(param.type)) {
      decl_command += "std::move(" + param.name + ')';
      if (param.type.find("chrono") != string::npos) {
        cerr << "not implemented (move + chrono)" << endl;
        abort();
      }
    } else {
      decl_command += param.name;
      if (param.type.find("chrono") != string::npos) decl_command += ".count()";
    }
  }
  decl_command += "}, ";
  decl_command += "shard";
  decl_command += ", ";
  decl_command += master ? "true" : "false";
  decl_command += ", GetCommandControl(command_control)));";
  decl_command += decl_ending;

  process_file(client_impl_cpp_filename, decl_command);
}

void process_request_file() {
  static const string request = "using Request";
  static const string decl_ending = ";";
  cerr << "--- " << __func__ << " ---" << endl;
  string request_filename_full = add_path(uservices_path, request_filename);
  string request_file = read_file(request_filename_full);
  //	cerr << request_file << endl;

  vector<size_t> using_starts;
  vector<size_t> reqtype_names_pos;
  vector<string> reqtype_names;

  for (size_t pos = 0;;) {
    pos = request_file.find(request, pos + 1);
    if (pos == string::npos) break;
    using_starts.emplace_back(pos);

    reqtype_names_pos.emplace_back(pos + request.size());
    reqtype_names.emplace_back(
        request_file.substr(reqtype_names_pos.back(),
                            request_file.find(' ', reqtype_names_pos.back()) -
                                reqtype_names_pos.back()));
  }
  assert(!using_starts.empty());
  using_starts.emplace_back(request_file.find(';', using_starts.back()) + 2);

  assert(reqtype_names_pos.size() == reqtype_names.size());
  assert(reqtype_names_pos.size() + 1 == using_starts.size());
  for (size_t i = 1; i < reqtype_names.size(); i++) {
    assert(reqtype_names[i - 1] <= reqtype_names[i]);
  }

  cerr << "current_reqtype_count=" << reqtype_names.size() << endl;
  for (const auto& reqtype : reqtype_names) {
    cerr << reqtype << endl;
  }

  size_t idx =
      lower_bound(reqtype_names.begin(), reqtype_names.end(), command_cc) -
      reqtype_names.begin();
  //	if (idx < reqtype_names.size()) {
  //		cerr << "reqtype_names[idx] = \"" << reqtype_names[idx] << '"'
  //<< endl;
  //	}
  if (idx >= reqtype_names.size() || reqtype_names[idx] != command_cc) {
    string decl_reqtype = "using Request" + command_cc + " = Request<" +
                          request_tmpl_param + ">;\n";
    cerr << decl_reqtype << endl;
    request_file.insert(using_starts[idx], decl_reqtype);
  }

  write_file(request_filename_full, request_file);
}

void process_transaction_hpp_file(const std::string& transaction_filename,
                                  bool add_equal_zero) {
  cerr << "--- " << __func__ << " ---" << endl;

  string decl_command =
      "  virtual Request" + command_cc + ' ' + command_cc + '(';
  bool first = true;
  for (const auto& param : params) {
    if (first)
      first = false;
    else
      decl_command += ", ";
    decl_command += param.type + ' ' + param.name;
  }
  if (add_equal_zero)
    decl_command += equal_zero;
  else
    decl_command += not_equal_zero;

  process_file(transaction_filename, decl_command);
}

void process_transaction_impl_hpp_file(
    const std::string& transaction_filename) {
  cerr << "--- " << __func__ << " ---" << endl;

  string decl_command = "  Request" + command_cc + ' ' + command_cc + '(';
  bool first = true;
  for (const auto& param : params) {
    if (first)
      first = false;
    else
      decl_command += ", ";
    decl_command += param.type + ' ' + param.name;
  }
  decl_command += override_ending;

  process_file(transaction_filename, decl_command);
}

string gen_update_shard() {
  string decl_command;
  bool shard_found = is_shard_found();
  if (!shard_found) {
    if (params[0].name == "key") {
      for (const auto& param : params) {
        if (param.type.find("std::vector") == 0 &&
            (param.name == "members" || param.name == "fields")) {
          // TODO: add assert or dummy request
          //					add_dummy_request(param.name,
          // decl_command);
        }
      }
      decl_command += "  UpdateShard(key);\n";
    } else if (params[0].name == "keys" || params[0].name == "key_values") {
      // TODO: add assert or dummy request
      //			add_dummy_request(params[0].name, decl_command);
      decl_command += "  UpdateShard(" + params[0].name + ");\n";
    } else {
      cerr << "can't determine shard by params" << endl;
      abort();
    }
  } else {
    decl_command += "  UpdateShard(shard);\n";
  }
  return decl_command;
}

void process_transaction_impl_cpp_file() {
  cerr << "--- " << __func__ << " ---" << endl;

  string decl_command = gen_first_line_cpp("TransactionImpl", false, false);
  decl_command += gen_update_shard();
  decl_command +=
      "  return AddCmd<Request" + command_cc + ">(\"" + command + '"';
  for (const auto& param : params) {
    if (param.name == "shard") continue;
    decl_command += ", ";
    if (need_move(param.type)) {
      decl_command += "std::move(" + param.name + ')';
      if (param.type.find("chrono") != string::npos) {
        cerr << "not implemented (move + chrono)" << endl;
        abort();
      }
    } else {
      decl_command += param.name;
      if (param.type.find("chrono") != string::npos) decl_command += ".count()";
    }
  }

  decl_command += ");\n}";

  process_file(transaction_impl_cpp_filename, decl_command);
}

void process_mock_client_base_test_hpp_file() {
  cerr << "--- " << __func__ << " ---" << endl;

  string decl_command = "  Request" + command_cc + ' ' + command_cc + '(';
  for (const auto& param : params) {
    decl_command += param.type + ' ' + param.name + ", ";
  }
  decl_command += "const CommandControl& command_control" + override_ending;

  process_file(mock_client_base_test_hpp_filename, decl_command);
}

void process_mock_client_base_test_cpp_file() {
  cerr << "--- " << __func__ << " ---" << endl;

  static const string decl_ending = "\n}";

  string decl_command =
      "Request" + command_cc + " MockClientBase::" + command_cc + '(';
  for (const auto& param : params) {
    decl_command += param.type + " /*" + param.name + "*/, ";
  }
  decl_command += "const CommandControl& /*command_control*/) {\n";
  decl_command += "  UASSERT_MSG(false, \"redis method not mocked\");\n";
  decl_command += "  return Request" + command_cc + "{nullptr};";
  decl_command += decl_ending;

  process_file(mock_client_base_test_cpp_filename, decl_command);
}

void process_mock_transaction_test_cpp_file() {
  cerr << "--- " << __func__ << " ---" << endl;

  static const string decl_ending = "\n}";

  string decl_command = gen_first_line_cpp("MockTransaction", false, false);
  decl_command += gen_update_shard();
  decl_command += "  return AddSubrequest(impl_->" + command_cc + '(';
  bool first = true;
  for (const auto& param : params) {
    if (first)
      first = false;
    else
      decl_command += ", ";
    if (need_move(param.type)) {
      decl_command += "std::move(" + param.name + ')';
      if (param.type.find("chrono") != string::npos) {
        cerr << "not implemented (move + chrono)" << endl;
        abort();
      }
    } else {
      decl_command += param.name;
    }
  }
  decl_command += "));";
  decl_command += decl_ending;

  process_file(mock_transaction_test_cpp_filename, decl_command);
}

void process_mock_transaction_impl_base_test_cpp_file() {
  cerr << "--- " << __func__ << " ---" << endl;

  static const string decl_ending = "\n}";

  string decl_command =
      gen_first_line_cpp("MockTransactionImplBase", false, true);
  decl_command += "  UASSERT_MSG(false, \"redis method not mocked\");\n";
  decl_command += "  return Request" + command_cc + "{nullptr};";
  decl_command += decl_ending;

  process_file(mock_transaction_impl_base_test_cpp_filename, decl_command);
}

void print_params(const vector<param_t>& params) {
  for (const auto& param : params) {
    cerr << "type='" << param.type << '\'' << "  name='" << param.name << '\''
         << endl;
  }
}

void strip(string& s) {
  while (!s.empty() && isspace(s.front())) s = s.substr(1);
  while (!s.empty() && isspace(s.back())) s.pop_back();
}

vector<param_t> parse_params(const char* begin, const char* end) {
  vector<param_t> params;

  cerr << "parse_params(" << string(begin, end) << ')' << endl;

  int open_br = 0;
  string type;
  string name;
  auto make_name = [&] {
    auto sp_pos = type.rfind(' ');
    assert(sp_pos != string::npos);
    while (sp_pos + 1 < type.size() && !isalpha(type[sp_pos + 1]) &&
           type[sp_pos + 1] != '_')
      ++sp_pos;
    assert(sp_pos + 1 < type.size());
    name = type.substr(sp_pos + 1);
    type.resize(sp_pos + 1);
    strip(type);
    strip(name);
  };

  for (auto cur = begin; cur != end; cur++) {
    char ch = *cur;
    if (ch == '<') ++open_br;
    if (ch == '>') --open_br;
    if (ch == ',' && !open_br) {
      make_name();
      params.emplace_back(param_t{std::move(type), std::move(name)});
      type.clear();
      name.clear();
    } else {
      type += ch;
    }
  }

  strip(type);
  if (!type.empty()) {
    make_name();
    assert(!type.empty());
    assert(!name.empty());
    params.emplace_back(param_t{std::move(type), std::move(name)});
  }

  return params;
}

[[noreturn]] void usage(const char* cmd) {
  string fn = cmd;
  auto pos = fn.rfind('/');
  if (pos != string::npos) fn = fn.substr(pos + 1);
  fn = "./" + fn;
  cerr << "usage: " << fn
       << " command request_template_param command_arguments [--dry-run]"
       << endl;
  cerr << "example: " << fn
       << " zrange 'std::vector<std::string>' 'std::string key, int64_t start, "
          "int64_t stop' --dry-run"
       << endl;
  cerr << "example: " << fn
       << " hmset 'StatusOk, void' 'std::string key, "
          "std::vector<std::pair<std::string, std::string>> field_values'"
       << endl;
  exit(1);
}

void check_usage(int argc, char** argv) {
  if (argc <= 3) usage(argv[0]);
  string cmd = argv[0];
  auto pos = cmd.rfind('/');
  if (pos != string::npos) {
    if (cmd.substr(0, pos) != ".") usage(argv[0]);
  }
}

int main(int argc, char** argv) {
  check_usage(argc, argv);
  uservices_path = default_uservices_path;
  command = argv[1];
  command_cc = command;
  assert(command_cc.size() > 0);
  command_cc[0] = toupper(command_cc[0]);

  request_tmpl_param = argv[2];
  params = parse_params(argv[3], argv[3] + strlen(argv[3]));
  if (argc > 4) {
    if (!strcmp(argv[4], "--dry-run")) dry_run = true;
  }

  int call_res =
      system(("redis-cli -h taximeter-base-redis-myt-01.taxi.dev.yandex.net -p "
              "6379 -a taximeter-dev command info " +
              command + " | grep 'write' >" + tmp_filename)
                 .c_str());
  cerr << "call_res=" << call_res << endl;
  string write_str;
  master = !!(ifstream(tmp_filename) >> write_str);
  cerr << "master=" << master << endl;

  print_params(params);

  process_client_file();
  process_client_impl_hpp_file();
  process_client_impl_cpp_file();
  process_request_file();
  process_transaction_hpp_file(transaction_hpp_filename, true);
  process_transaction_impl_hpp_file(transaction_impl_hpp_filename);
  process_transaction_impl_cpp_file();
  process_mock_client_base_test_hpp_file();
  process_mock_client_base_test_cpp_file();
  process_transaction_impl_hpp_file(mock_transaction_test_hpp_filename);
  process_mock_transaction_test_cpp_file();
  process_transaction_hpp_file(mock_transaction_impl_base_test_hpp_filename,
                               false);
  process_mock_transaction_impl_base_test_cpp_file();

  return 0;
}
