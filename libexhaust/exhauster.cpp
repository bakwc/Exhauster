#include <assert.h>
#include <algorithm>
#include <limits>
#include <boost/algorithm/string.hpp>

#include <contrib/htmlcxx/html/utils.h>
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
    } else if (word.size() == 8) {
        return (isdigit(word[0]) && isdigit(word[1]) &&
                        isdigit(word[3]) && isdigit(word[4]) &&
                        isdigit(word[6]) && isdigit(word[7]) &&
                isdatedelim(word[2]) && isdatedelim(word[5]));
    } else if ((word.size() == 10 && CalcDigitCount(word) == 8) ||
               word.size() == 5 && CalcDigitCount(word) == 4)
    {
        return true;
    }
    return false;
}

// filter element by text (currently only dates filtered)
bool TextFiltered(const string& text) {
    string normalizedText = NormalizeText(DecodeHtmlEntities(text), false);
    vector<string> words;
    boost::algorithm::split(words, normalizedText, boost::algorithm::is_any_of(" "));

    if (words.size() == 1) {
        return false;
    }

    size_t dateWords = 0;
    size_t wordsSize = 0;

    for (size_t i = 0; i < words.size(); i++) {
        if (words[i].size() == 1) {
            continue;
        }
        wordsSize++;
        if (CanBeDate(words[i])) {
            dateWords++;
        }
    }
    if (dateWords > 0 && dateWords >= wordsSize / 2) {
        return true;
    }
    if (dateWords >= 2 && dateWords >= wordsSize / 3) {
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
                name == "author" || name == "copyrights" || name == "hidden" ||
                name == "hide" || name == "dablink" || name == "comment" ||
                name == "comments")
        {
            return true;
        }
    }
    return false;
}

// filter dom tree element by `style`
bool StyleFiltered(const string& style) {
    if (style.find("display:none") != string::npos) {
        return true;
    }
    return false;
}

bool IsHeader(const string& tag) {
    if (tag.size() == 2 && tag[0] == 'h' && isdigit(tag[1])) {
        return true;
    }
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
bool HasInterestingContent(tree<HTML::Node>::iterator it,
                           string parentTag,
                           bool hard,
                           size_t* words)
{
    string tag = GetTag(it);
    size_t wordsThreshold = 4;

    if (!hard && it->isComment()) {
        return false;
    }

    if (tag == "option" || tag == "a" || IsHeader(tag)) {
        return false;
    }

    if (parentTag == "li") {
        wordsThreshold = 21;
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
        size_t punctsCnt = CalcPunctCount(text);
        if (words) {
            *words += wordsCnt;
        }
        if (hard) {
            return (wordsCnt >= wordsThreshold && HasPunct(text)) ||
                    wordsCnt >= 2 && parentTag == "b";
        } else {
            return !TextFiltered(text) && (wordsCnt >= 1 || punctsCnt >= 1);
            return true;
        }
    }

    bool result = false;
    tree<HTML::Node>::iterator jt = it.begin();
    for (;jt != it.end(); jt++) {
        result |= HasInterestingContent(jt, tag, hard, words);
        jt.skip_children();
    }

    return result;
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
                tag == "noscript" || tag == "title" || tag == "meta" ||
                tag == "link") // "ul" should stay
        {
            return true;
        }

        if (it.number_of_children() == 0) {
            return (tag != "img");
        }

        if (!node.text().empty()) {
            node.parseAttributes();
            if (node.attribute("onclick").first) {
                return true;
            }
        }

        if (ClassFiltered(node.attribute("class").second) ||
            ClassFiltered(node.attribute("id").second) ||
            StyleFiltered(node.attribute("style").second))
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

        if ((tag == "div" || tag == "table" ||
             tag == "ul" || tag == "p") &&
                it.number_of_children() >= 1)
        {
            if (!HasInterestingContent(it, tag)) {
                return true;
            }
        }

        if (it.number_of_children() == 1) {
            string childTag = GetTag(it.begin());
            if (tag == "div" && childTag == "div") {
                size_t words = 0;
                HasInterestingContent(it.begin(), tag, false, &words);
                if (words < 12) {
                    return true;
                }
            }
        }

        return false;
    } else {
        return !HasInterestingContent(it, "", false);
    }
}

bool GetCharset(tree<HTML::Node>::iterator it, string& charset) {
    HTML::Node& node = *it;
    if (node.isTag()) {
        string tag = GetTag(it);
        if (tag == "meta") {
            node.parseAttributes();
            map<string, string> attributes = node.attributes();
            map<string, string>::iterator it;
            bool hasContentType = false;
            string contentType;
            for (it = attributes.begin(); it != attributes.end(); it++) {
                string key = it->first;
                string value = it->second;
                boost::algorithm::to_lower(key);
                boost::algorithm::to_lower(value);
                if (key == "http-equiv" && value == "content-type") {
                    hasContentType = true;
                }
                if (key == "content") {
                    contentType = value;
                }
            }
            if (hasContentType) {
                vector<string> params;
                boost::algorithm::split(params, contentType, boost::algorithm::is_any_of("; "));
                for (size_t i = 0; i < params.size(); i++) {
                    if (boost::algorithm::starts_with(params[i], "charset=")) {
                        charset = params[i].substr(8);
                        return true;
                    }
                }
            }
        }

        if (tag != "html" && !tag.empty()) {
            return false;
        }

        for (tree<HTML::Node>::iterator jt = it.begin(); jt != it.end(); jt++) {
            if (GetCharset(jt, charset)) {
                return true;
            }
        }
    }
    return false;
}

bool GetTitle(tree<HTML::Node>::iterator it, string& title) {
    HTML::Node& node = *it;
    if (node.isTag()) {
        string tag = GetTag(it);
        if (tag == "title") {
            if (it.number_of_children() == 1) {
                title = it.begin()->text();
                return true;
            }
        } else if (tag != "html" && !tag.empty()) {
            return false;
        }

        for (tree<HTML::Node>::iterator jt = it.begin(); jt != it.end(); jt++) {
            if (GetTitle(jt, title)) {
                return true;
            }
        }
    }
    return false;
}

void DecodeTree(tree<HTML::Node>& dom, string charsetFrom) {
    for (tree<HTML::Node>::iterator it = dom.begin(); it != dom.end(); it++) {
        HTML::Node& node = *it;
        string decodedText = RecodeText(node.text(), charsetFrom, "UTF-8");
        if (!decodedText.empty()) {
            node.text(decodedText);
        }
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
        if (IsHeader(tag)) {
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

void MakeBlocks(vector<TContentBlock>& blocks,
                TElements& elements,
                string title)
{
    TContentBlock current;
    if (elements.empty()) {
        blocks.push_back(current);
        return;
    }
    string prevPath = elements.begin()->Path;
    current.Path = prevPath;
    for (TElements::iterator it = elements.begin(); it != elements.end(); it++) {
        bool skip = false;
        string currentText = it->Text;
        if (CalcWordsCount(currentText) > 2 &&
                title.find(currentText) != string::npos)
        {
            it->Type |= TP_Header;
        }
        currentText = DecodeHtmlEntities(currentText);

        float distance = numeric_limits<float>::max();
        TElements::const_iterator jt = it;
        jt++;
        if (jt != elements.end()) {
            distance = GetPathDistance(it->Path, jt->Path);
        }

        if (distance > 1.6) {
            currentText = ImproveText(currentText);
        }

        bool makeNewBlock = false;
        makeNewBlock |= GetPathDistance(prevPath, it->Path) >= 5;

        makeNewBlock |= (it->Type & TP_Header &&
                        jt != elements.end() &&
                        jt->Path != prevPath);

        if (it->Type & TP_Header && !makeNewBlock) {
            current.Repeated = true;
        }

        if (makeNewBlock) {
            if (CalcWordsCount(current.Text) > 6) {
                current.Text = ImproveText(current.Text);
                blocks.push_back(current);
            }
            current = TContentBlock();
            current.Path = it->Path;
            prevPath = it->Path;
            if (it->Type & TP_Header) {
                current.Title = currentText;
            }
        }

        if (current.Text.empty()) {
            // skips links at the begining of block
            if (it->Type & TP_Link) {
                skip = true;
            }
        }

        if ((it->Type & TP_Text) &&
                (!makeNewBlock || !(it->Type & TP_Header)) &&
                !skip)
        {
            current.Text += currentText + " "; // todo: add dot if required
            current.Path = GetCommonPath(current.Path, it->Path);
            prevPath = it->Path;
        }
        if (it->Type & TP_Link) {
            current.Links.push_back(currentText);
        }
    }

    if (CalcWordsCount(current.Text) > 6) {
        current.Text = ImproveText(current.Text);
        blocks.push_back(current);
    }

    if (blocks.empty()) {
        blocks.push_back(TContentBlock());
    }
}

void PrepareBlocks(vector<TContentBlock>& blocks, const string& title) {
    size_t mainBlock = 0;
    if (blocks.size() >= 2) {
        // Если второй блок - больше чем первый, то
        // либо клеим первый блок ко второму,
        // либо вообще удаляем первый блок
        if (blocks[1].Text.size() > blocks[0].Text.size() * 2) {
            if (CalcWordsCount(blocks[0].Text) > 6 & blocks[0].Links.empty()) {
                blocks[0].Text += " " + blocks[1].Text;
                blocks[1].Type = BT_Removed;
            } else {
                blocks[0].Type = BT_Removed;
                mainBlock = 1;
            }
        }

        // пропускаем первые несколько блоков с одинаковым path
        // и берём следующий после них, в случае если у него много
        // контента
        if (blocks[0].Path == blocks[1].Path) {
            size_t c = 1;
            while (c < blocks.size() &&
                   blocks[c].Path == blocks[c - 1].Path)
            {
                c++;
            }
            if (c != blocks.size() &&
                    blocks[c].Type != BT_Removed &&
                    blocks[c].Text.size() > blocks[mainBlock].Text.size() * 2)
            {
                mainBlock = c;
            }
        }

        // Если заголовок второго блока совпадает с заголовком страницы
        // то считаем его основным контентом
        if (mainBlock == 0 &&
                blocks[1].Type != BT_Removed)// &&
                //!blocks[1].Repeated)
        {
            if (!blocks[1].Title.empty() &&
                    !title.empty() &&
                    (blocks[1].Title.find(title) != string::npos ||
                    title.find(blocks[1].Title) != string::npos))
            {
                mainBlock = 1;
                blocks[1].Type = BT_MainContent;
                blocks[0].Type = BT_AdditionalContent;
            }
        }
    }
    blocks[mainBlock].Type = BT_MainContent;
    if (blocks[mainBlock].Title.empty()) {
        blocks[mainBlock].Title = title;
    }
}

TContentBlock ExhausteMainContent(const string& htmlData,
                           const TSettings& settings)
{
    vector<TContentBlock> blocks = ExhausteContent(htmlData, settings);
    for (size_t i = 0; i < blocks.size(); i++) {
        if (blocks[i].Type == BT_MainContent) {
            return blocks[i];
        }
    }
}


vector<TContentBlock> ExhausteContent(const string& htmlData,
                               const TSettings& settings)
{
    HTML::ParserDom parser;
    tree<HTML::Node> dom = parser.parseTree(htmlData);

    string charset = settings.Charset;
    string title;

    GetCharset(dom.begin(), charset);
    GetTitle(dom.begin(), title);

    Filter(dom, dom.begin());
    if (charset != "utf8" && charset != "utf-8") {
        DecodeTree(dom, charset);
        string newTitle = RecodeText(title, charset, "UTF-8");
        if (!newTitle.empty()) {
            title = newTitle;
        }
    }

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
    MakeBlocks(blocks, elements, title);

    if (settings.DebugOutput) {
        DumpBlocks(blocks, *settings.DebugOutput);
    }

    PrepareBlocks(blocks, title);

    if (settings.DebugOutput) {
        DumpBlocks(blocks, *settings.DebugOutput);
    }

    assert(!blocks.empty() && "no blocks returned");
    return blocks;
}

} // NExhauster
