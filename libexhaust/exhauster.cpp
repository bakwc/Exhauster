#include <assert.h>
#include <algorithm>
#include <boost/algorithm/string.hpp>

#include <utils/string.h>

#include "exhauster.h"

using namespace std;

namespace NExhauster {

// todo: rewrite using regexps... or not
bool CanBeDate(const string& word) {
    if (word.size() == 2 || word.size() == 4) {
        size_t digits = 0;
        for (size_t i = 0; i < word.size(); i++) {
            if (isdigit(word[i])) {
                digits ++;
            }
        }
        if (digits == word.size()) {
            return true;
        }
    }
    if (word.size() == 8) {
        return (isdigit(word[0]) && isdigit(word[1]) &&
                        isdigit(word[3]) && isdigit(word[4]) &&
                        isdigit(word[6]) && isdigit(word[7]) &&
                        (word[2] == '.' || word[2] == ':') &&
                        (word[5] == '.' || word[5] == ':'));
    }
    return false;
}

// filter element by text (currently only dates filtered)
bool TextFiltered(const string& text) {
    string normalizedText = NormalizeText(text, false);
    vector<string> words;
    boost::algorithm::split(words, normalizedText, boost::algorithm::is_any_of(" "));

    size_t dateWords = 0;
    for (size_t i = 0; i < words.size(); i++) {
        if (CanBeDate(words[i])) {
            dateWords++;
        }
    }
    if (dateWords > 0 && dateWords >= words.size() / 2) {
        return true;
    }
    return false;
}

// filer dom-tree element by `class` or `id` attribute value
bool ClassFiltered(const string& name) {
    vector<string> names;
    boost::algorithm::split(names, name, boost::algorithm::is_any_of(" "));

    for (size_t i = 0; i < names.size(); i++) {
        const string& name = names[i];
        // todo: dehardcode.. or not
        if (name == "twitter-content" || name == "twitter-post" ||
                name == "copyright" || name == "footer" || name == "time" ||
                name == "author" || name == "copyrights" || name == "hidden")
        {
            return true;
        }
    }
    return false;
}

string GetTag(tree<HTML::Node>::iterator it) {
    string tag;
    HTML::Node& node = *it;
    if (node.isTag()) {
        tag = node.tagName();
        tag.erase(std::remove(tag.begin(), tag.end(), '\n'), tag.end());
        boost::algorithm::to_lower(tag);
    }
    return tag;
}

/** @brief detects if element (and subelements) has interesting content in it
  * @param hard - true for single elements, false to check elements with child
  */
bool HasInterestingContent(tree<HTML::Node>::iterator it, bool hard, size_t* words) {
    string tag = GetTag(it);
    if (!hard && (tag == "a" || it->isComment() ||
            tag == "li"))
    {
        return false;
    }

    if (tag == "option" ) {
        return false;
    }

    if (hard && tag == "img") {
        return true;
    }

    if (it.number_of_children() == 0) {
        if (it->isTag()) {
            return false;
        }
        string text = it->text() + " ";
        replace(text.begin(), text.end(), '\n', ' ');
        replace(text.begin(), text.end(), '\t', ' ');
        size_t wordsCnt = CalcWordsCount(text);
        if (words) {
            *words += wordsCnt;
        }
        if (hard) {
            return wordsCnt >= 4 && text.find(".") != string::npos;
        } else {
            return !TextFiltered(text) && wordsCnt >= 1;
        }
    }

    bool result = false;
    tree<HTML::Node>::iterator jt = it.begin();
    for (;jt != it.end(); jt++) {
        result |= HasInterestingContent(jt, hard, words);
        jt.skip_children();
    }

    return result;
}

// check if element's tag is `parent` and it has child with tag `child`
bool CheckPair(const string& parent,
                const string& child,
                const string& tag,
                tree<HTML::Node>::iterator it)
{
    if (tag == parent) {
        for (tree<HTML::Node>::iterator jt = it.begin(); jt != it.end(); jt++) {
            if (GetTag(jt) != child) {
                return false;
            }
            if (HasInterestingContent(jt)) {
                return false;
            }
            jt.skip_children();
        }
        return true;
    }
    return false;
}

/** @brief filters element and sub-elements;
  *   removes filtered elements from tree
  */
bool Filter(tree<HTML::Node>& dom, tree<HTML::Node>::iterator it) {
    HTML::Node& node = *it;
    if (node.isComment()) {
        return true;
    }
    if (node.isTag()) {
        string tag = GetTag(it);

        if (tag == "head" || tag == "script" || tag == "input" ||
                tag == "iframe" || tag == "frame" || tag == "img" ||
                tag == "style" || tag == "textarea" || tag == "footer" ||
                tag == "aside" || tag == "select" || tag == "option" ||
                tag == "noscript") // "ul" should stay
        {
            return true;
        }

        if (it.number_of_children() == 0) {
            return (tag != "img");
        }

        node.parseAttributes();
        if (node.attribute("onclick").first) {
            return true;
        }

        if (ClassFiltered(node.attribute("class").second) ||
            ClassFiltered(node.attribute("id").second))
        {
             return true;
        }

        bool result = true;
        tree<HTML::Node>::iterator jt;
        for (jt = it.begin(); jt != it.end();) {
            if (Filter(dom, jt)) {
                jt = dom.erase(jt);
            } else {
                result = false;
                jt.skip_children();
                jt++;
            }
        }
        if (result) {
            return true;
        }

        // todo: remove if unused
        if (it.number_of_children() > 1) {
            result = false;
            result |= CheckPair("div", "div", tag, it);
            //result |= check_pair("ul", "li", tag, it);
            if (result) {
                return true;
            }
        }

        if ((tag == "div" || tag == "table") && it.number_of_children() >= 1) {
            if (!HasInterestingContent(it)) {
                return true;
            }
        }

        if (it.number_of_children() == 1) {
            string childTag = GetTag(it.begin());
            if (tag == "div" && childTag == "div") {
                size_t words = 0;
                HasInterestingContent(it.begin(), false, &words);
                if (words < 12) {
                    return true;
                }
            }
        }

        return false;
    } else {
        return !HasInterestingContent(it, false);
    }
}


///  ---- paths ----

void AddDistance(const string& path, size_t from, float& distance) {
    for (size_t i = from; i < path.size(); i++) {
        if (path[i] == '/') {
            if (i + 1 < path.size() && path[i + 1] == 'p') {
                distance += 0.5;
            } else {
                distance += 1;
            }
        }
    }
}

float GetPathDistance(const string& path1, const string& path2) {
    size_t len = max(path1.size(), path2.size());
    float distance = 0;
    size_t i;
    for (i = 0; i < len; i++) {
        if (path1[i] != path2[i]) {
            break;
        }
    }
    AddDistance(path1, i, distance);
    AddDistance(path2, i, distance);
    return distance;
}

string GetCommonPath(const string& path1, const string& path2) {
    assert(path1.size() > 0 && path2.size() > 0 && "paths should not be empty");
    size_t len = max(path1.size(), path2.size());
    size_t i = 0;
    for (; i < len; i++) {
        if (path1[i] != path2[i]) {
            break;
        }
    }
    i = path1.rfind('/', i + 1);
    assert(i != string::npos && "there should be at least 1 /");
    return path1.substr(0, i + 1);
}

void MakeElements(TElements& elements,
                      const tree<HTML::Node>& dom,
                      string path,
                      unsigned char elementType)
{
    tree<HTML::Node>::iterator it = dom.begin();
    if (!it->isTag()) {
        TElement element;
        element.Type = elementType;
        element.Text = it->text();
        element.Path = path;
        elements.push_back(element);
    } else {
        string tag = GetTag(it);
        path += tag;
        if (tag != "html") {
            it->parseAttributes();
            map<string, string> attributes = it->attributes();
            for (map<string, string>::iterator it = attributes.begin(); it != attributes.end();) {
                if (it->first.empty() || it->second.empty() ||
                        it->first == "href") // todo: apply tolower to attributes
                {
                    it++;
                    continue;
                }
                if (it == attributes.begin()) {
                    path += ":";
                }
                path += it->first + "=" + it->second;
                if (++it != attributes.end()) {
                    path += ";";
                }
            }
        }
        path += "/";
        if (tag.size() == 2 && tag[0] == 'h' && isdigit(tag[1])) {
            elementType |= TP_Header;
        }
        if (tag == "a") {
            elementType |= TP_Link;
        }
    }
    for (unsigned i = 0; i < dom.number_of_children(it); i++) {
        MakeElements(elements, dom.child(it, i), path, elementType);
    }
}

void TrimHeaderElements(TElements& elements) {
    // trim start headers
    for (TElements::iterator it = elements.begin(); it != elements.end();) {
        TElements::iterator jt = it;
        jt++;
        if (!(it->Type & TP_Header)) {
            break;
        } else if (jt != elements.end() &&
                jt->Type & TP_Header)
        {
            it = elements.erase(it);
        } else {
            it++;
        }
    }

    // trim headers at the end
    TElements::iterator it = elements.end();
    for (it--; it != elements.begin();) {
        if (!(it->Type & TP_Header)) {
            break;
        }
        it = elements.erase(it);
        it--;
    }
}

void MakeBlocks(vector<TContentBlock>& blocks, const TElements& elements) {
    TContentBlock current;
    string elementPath = elements.begin()->Path;
    current.Path = elementPath;
    for (TElements::const_iterator it = elements.begin(); it != elements.end(); it++) {
        if (it->Type & TP_Header || GetPathDistance(elementPath, it->Path) > 5) {
            if (CalcWordsCount(current.Text) > 6) {
                blocks.push_back(current);
            }
            current = TContentBlock();
            current.Path = it->Path;
            elementPath = it->Path;
            if (it->Type & TP_Header) {
                current.Title = it->Text;
            }
        }
        if (it->Type & TP_Text) {
            current.Text += it->Text + " "; // todo: add dot if required
            current.Path = GetCommonPath(current.Path, it->Path);
            elementPath = it->Path;
        }
    }

    if (CalcWordsCount(current.Text) > 6) {
        blocks.push_back(current);
    }
}

TContentBlock ExhausteMainContent(const string& htmlData,
                           const TSettings& settings)
{
    return ExhausteContent(htmlData, settings)[0];
}


vector<TContentBlock> ExhausteContent(const string& htmlData,
                               const TSettings& settings)
{
    HTML::ParserDom parser;
    tree<HTML::Node> dom = parser.parseTree(htmlData);

    Filter(dom, dom.begin());

    if (settings.DebugOutput) {
        DumpTree(dom, *settings.DebugOutput);
        *settings.DebugOutput << "\n\n";
    }

    TElements elements;

    MakeElements(elements, dom);
    TrimHeaderElements(elements);

    if (settings.DebugOutput) {
        DumpElements(elements, *settings.DebugOutput);
        *settings.DebugOutput << "\n\n";
    }

    vector<TContentBlock> blocks;
    MakeBlocks(blocks, elements);

    if (settings.DebugOutput) {
        DumpBlocks(blocks, *settings.DebugOutput);
    }

    assert(!blocks.empty() && "no blocks returned");
    return blocks;
}

} // NExhauster
