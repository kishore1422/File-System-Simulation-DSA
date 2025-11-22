#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>

using namespace std;

/*
   Simple File System Simulator (C++)
   Now supports:
   - cat <file>
   - edit <file>
*/

struct Node {
    string name;
    bool isFile;
    string content;          // NEW: file content
    Node* parent;
    vector<Node*> children;

    Node(const string& n, bool file=false, Node* p=nullptr)
        : name(n), isFile(file), parent(p) {}

    Node* findChild(const string& childName) {
        for (Node* c : children)
            if (c->name == childName) return c;
        return nullptr;
    }
};

class FileSystem {
private:
    Node* root;
    Node* cwd;

    vector<string> splitPath(const string& path) {
        vector<string> parts;
        string cur;
        for (char ch : path) {
            if (ch == '/') {
                if (!cur.empty()) {
                    parts.push_back(cur);
                    cur.clear();
                }
            } else cur.push_back(ch);
        }
        if (!cur.empty()) parts.push_back(cur);
        return parts;
    }

    string getPath(Node* node) {
        if (node == root) return "/";
        vector<string> parts;
        while (node && node != root) {
            parts.push_back(node->name);
            node = node->parent;
        }
        reverse(parts.begin(), parts.end());
        string p = "/";
        for (int i = 0; i < parts.size(); i++) {
            p += parts[i];
            if (i + 1 < parts.size()) p += "/";
        }
        return p;
    }

    pair<Node*, vector<string>> resolve(const string& path, bool mustExist=true) {
        Node* cur = (path.size() > 0 && path[0] == '/') ? root : cwd;
        vector<string> parts = splitPath(path);

        for (int i = 0; i < parts.size(); i++) {
            if (parts[i] == ".") continue;
            if (parts[i] == "..") {
                if (cur->parent) cur = cur->parent;
                continue;
            }
            Node* child = cur->findChild(parts[i]);
            if (!child) {
                vector<string> leftover(parts.begin() + i, parts.end());
                if (mustExist) return {nullptr, {}};
                return {cur, leftover};
            }
            cur = child;
        }
        return {cur, {}};
    }

    void deleteRec(Node* node) {
        for (Node* c : node->children) deleteRec(c);
        delete node;
    }

    void printTree(Node* node, string prefix="", bool isLast=true) {
        if (node == root) cout << "/\n";
        else {
            cout << prefix << (isLast ? "└── " : "├── ") << node->name;
            if (!node->isFile) cout << "/";
            cout << "\n";
        }
        for (int i = 0; i < node->children.size(); i++) {
            bool last = (i == node->children.size() - 1);
            string newPrefix = prefix + (isLast ? "    " : "│   ");
            printTree(node->children[i], newPrefix, last);
        }
    }

public:
    FileSystem() {
        root = new Node("", false, nullptr);
        cwd = root;
    }

    ~FileSystem() {
        deleteRec(root);
    }

    void mkdir(const string& path) {
        string parentPath, newName;
        size_t pos = path.find_last_of('/');
        if (pos == string::npos) parentPath = "", newName = path;
        else if (pos == 0) parentPath = "/", newName = path.substr(1);
        else parentPath = path.substr(0, pos), newName = path.substr(pos+1);

        auto res = resolve(parentPath, false);
        if (!res.first || !res.second.empty()) {
            cout << "Parent path does not exist.\n";
            return;
        }
        if (res.first->findChild(newName)) {
            cout << "Directory already exists.\n";
            return;
        }

        Node* d = new Node(newName, false, res.first);
        res.first->children.push_back(d);
        cout << "Directory created: " << getPath(d) << "\n";
    }

    void touch(const string& path) {
        string parentPath, newName;
        size_t pos = path.find_last_of('/');
        if (pos == string::npos) parentPath = "", newName = path;
        else if (pos == 0) parentPath = "/", newName = path.substr(1);
        else parentPath = path.substr(0, pos), newName = path.substr(pos+1);

        auto res = resolve(parentPath, false);
        if (!res.first) {
            cout << "Parent path does not exist.\n";
            return;
        }
        if (res.first->findChild(newName)) {
            cout << "File already exists.\n";
            return;
        }

        Node* f = new Node(newName, true, res.first);
        res.first->children.push_back(f);
        cout << "File created: " << getPath(f) << "\n";
    }

    void cat(const string& path) {
        auto res = resolve(path, true);
        if (!res.first) { cout << "File not found.\n"; return; }
        if (!res.first->isFile) { cout << "Not a file.\n"; return; }

        cout << "----- " << res.first->name << " -----\n";
        if (res.first->content.empty())
            cout << "(empty file)\n";
        else
            cout << res.first->content << "\n";
        cout << "-----------------------\n";
    }

    void edit(const string& path) {
        auto res = resolve(path, true);
        if (!res.first) { cout << "File not found.\n"; return; }
        if (!res.first->isFile) { cout << "Not a file.\n"; return; }

        cout << "Enter new content (type END on a new line to finish):\n";

        string text, line;
        while (true) {
            getline(cin, line);
            if (line == "END") break;
            text += line + "\n";
        }

        res.first->content = text;
        cout << "File updated.\n";
    }

    void cd(const string& path) {
        auto res = resolve(path, true);
        if (!res.first) { cout << "Path not found.\n"; return; }
        if (res.first->isFile) { cout << "Not a directory.\n"; return; }
        cwd = res.first;
    }

    void ls() {
        if (cwd->children.empty()) {
            cout << "(empty)\n";
            return;
        }
        vector<string> out;
        for (Node* c : cwd->children)
            out.push_back(c->isFile ? c->name : c->name + "/");

        sort(out.begin(), out.end());
        for (auto& s : out) cout << s << "  ";
        cout << "\n";
    }

    void pwd() { cout << getPath(cwd) << "\n"; }

    void rm(const string& path) {
        auto res = resolve(path, true);
        if (!res.first) { cout << "Path not found.\n"; return; }
        if (res.first == root) { cout << "Cannot delete root.\n"; return; }

        Node* n = res.first;
        Node* p = n->parent;
        p->children.erase(remove(p->children.begin(), p->children.end(), n), p->children.end());
        deleteRec(n);
        cout << "Removed.\n";
    }

    void tree() { printTree(cwd); }

    void search(const string& name) {
        vector<Node*> results;

        // DFS
        vector<Node*> stack = {root};
        while (!stack.empty()) {
            Node* cur = stack.back(); stack.pop_back();
            if (cur->name == name) results.push_back(cur);
            for (Node* c : cur->children) stack.push_back(c);
        }

        if (results.empty()) { cout << "Not found.\n"; return; }

        for (Node* n : results)
            cout << getPath(n) << (n->isFile ? "" : "/") << "\n";
    }

    void help() {
        cout << "Commands:\n"
             << " mkdir <path>\n"
             << " touch <path>\n"
             << " cat <file>\n"
             << " edit <file>\n"
             << " rm <path>\n"
             << " cd <path>\n"
             << " ls\n"
             << " pwd\n"
             << " tree\n"
             << " search <name>\n"
             << " help\n"
             << " exit\n";
    }

    string prompt() { return getPath(cwd) + " $ "; }
};

int main() {
    FileSystem fs;
    cout << "File System Simulator (C++)\nType 'help' for commands.\n";

    string line;
    while (true) {
        cout << fs.prompt();
        getline(cin, line);
        if (line.empty()) continue;

        stringstream ss(line);
        string cmd, arg;
        ss >> cmd;
        getline(ss, arg);
        if (arg.size() && arg[0] == ' ') arg = arg.substr(1);

        if (cmd == "exit") break;
        else if (cmd == "help") fs.help();
        else if (cmd == "mkdir") fs.mkdir(arg);
        else if (cmd == "touch") fs.touch(arg);
        else if (cmd == "cat") fs.cat(arg);
        else if (cmd == "edit") fs.edit(arg);
        else if (cmd == "rm") fs.rm(arg);
        else if (cmd == "cd") fs.cd(arg);
        else if (cmd == "ls") fs.ls();
        else if (cmd == "pwd") fs.pwd();
        else if (cmd == "tree") fs.tree();
        else if (cmd == "search") fs.search(arg);
        else cout << "Unknown command.\n";
    }

    cout << "Goodbye.\n";
}
