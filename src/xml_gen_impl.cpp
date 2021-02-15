#include <cstdarg>
#include <cstring>

#include "xml_gen_impl.h"

using namespace std;

static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

static std::string &replace(std::string &s, const std::string &from, const std::string &to) {
    if (!from.empty()) {
        for (size_t pos = 0; (pos = s.find(from, pos)) != std::string::npos; pos += to.size()) {
            s.replace(pos, from.size(), to);
        }
    }

    return s;
}

bool processing = false;
bool hasArgs = false;
bool isTypeInst = false;

int order = 1;
int argNum = 1;

void process_arg(string &str) {
    string type;

    int atPos = str.find('@');
    if (atPos == string::npos) {
        // No '@' -> it's either a type or a label
        if (isTypeInst) {
            type = "type";
        } else {
            type = "label";
        }
    } else {
        string firstPart = str.substr(0, atPos);
        string secondPart = str.substr(atPos + 1);

        if (firstPart == "GF" || firstPart == "LF" || firstPart == "TF") {
            type = "var";
        } else {
            type = replace(firstPart, "&", "&amp;");
            str = secondPart;
            str = replace(str, "&", "&amp;");
            str = replace(str, "<", "&lt;");
            str = replace(str, ">", "&gt;");
        }
    }

    cout << "    <arg" << argNum << " type=\"" << type << "\">" << str << "</arg" << argNum << ">\n";
    argNum++;
}

void out(string &str) {
    if(str == " ") return;

    ltrim(str);

    int spI = str.find(' ');
    bool hasWords = spI != string::npos;

    string firstWord = hasWords ? str.substr(0, spI) : str;

    if (processing) {
        process_arg(firstWord);
    } else {
        cout << "  <instruction opcode=\"" << firstWord << "\" order=\"" << order << "\"";
        processing = true;
        isTypeInst = firstWord == "TYPE";

        hasArgs = hasWords;
        if (hasWords) {
            cout << ">\n";
        }
    }

    if (hasWords) {
        str = str.substr(spI);
        if (str.length() > 1) {
            out(str);
        }
    }
}

void end() {
    if (hasArgs) {
        cout << "  </instruction>\n";
    } else {
        cout << " />\n";
    }

    processing = false;
    hasArgs = false;
    argNum = 1;
    order++;
}

extern "C" {
#include "xml_gen.h"

void xml_out(const char *format, ...) {
    char buf[1024]; // TODO: this is NOT ideal

    va_list arguments;
    va_start(arguments, format);
    vsprintf(buf, format, arguments);
    va_end(arguments);

    string s(buf);
    out(s);
}

void xml_nl() {
    end();
}

void xml_begin() {
    cout << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<program language=\"IPPcode21\">\n";
}

void xml_end() {
    cout << "</program>\n";
}
}

