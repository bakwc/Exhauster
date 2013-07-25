#include <assert.h>
#include <algorithm>
#include <limits>
#include <unordered_map>
#include <boost/algorithm/string.hpp>

#include <contrib/htmlcxx/html/utils.h>
#include <utils/string.h>

#include "exhauster.h"

using namespace std;

namespace NExhauster {

// todo: rewrite using regexps... or not
bool CanBeDate(const string& word) {
    if (word.size() == 4) {
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
    if (dateWords > 0 && dateWords > wordsSize / 2) {
        return true;
    }
    if (dateWords >= 2 && wordsSize > 3 && dateWords >= wordsSize / 3) {
        return true;
    }
    return false;
}

// filer dom-tree element by `class` or `id` attribute value
bool ClassFiltered(string name) {
    vector<string> names;
    boost::algorithm::to_lower(name);
    boost::algorithm::split(names, name, boost::algorithm::is_any_of(" "));

    for (size_t i = 0; i < names.size(); i++) {
        const string& name = names[i];
        // todo: dehardcode.. or not
        if (name == "twitter-content" || name == "twitter-post" ||
                name == "copyright" || name == "footer" || name == "time" ||
                name == "author" || name == "copyrights" || name == "hidden" ||
                name == "hide" || name == "dablink" || name == "comment" ||
                name == "for_users_only_msg")
        {
            return true;
        }
        if (name.find("slider") != string::npos ||
                name.find("havadurumu") != string::npos ||
                name.find("weather") != string::npos ||
                name.find("popup") != string::npos ||
                name.find("comments") != string::npos ||
                name.find("notice") != string::npos ||
                name.find("warning") != string::npos)
        {
            return true;
        }
    }
    return false;
}

// filter dom tree element by `style`
bool StyleFiltered(const string& style) {
    if (style.find("display:none") != string::npos ||
            style.find("display: none") != string::npos)
    {
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

    if (tag == "option" || tag == "a") {
        return false;
    }

    if (IsHeader(tag)) {
        wordsThreshold = 21;
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
            return ((wordsCnt >= wordsThreshold && HasPunct(text)) ||
                    (wordsCnt >= 2 && parentTag == "b")) &&
                    !TextFiltered(text);
        } else {
            return (wordsCnt >= 1 || punctsCnt >= 1);
        }
    }

    bool result = false;
    tree<HTML::Node>::iterator jt = it.begin();
    size_t newWords = 0;
    for (;jt != it.end(); jt++) {
        result |= HasInterestingContent(jt, tag, hard, &newWords);
        jt.skip_children();
    }

    if (newWords < wordsThreshold) {
        return false;
    }

    if (words) {
        *words += newWords;
    }

    if (result && IsHeader(tag)) {
        it->tagName("div");
    }

    return result;
}

/** @brief filters element and sub-elements;
  *   removes filtered elements from tree
  */
bool Filter(tree<HTML::Node>& dom,
            tree<HTML::Node>::iterator it,
            optional<tree<HTML::Node>::iterator> next)
{
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
        tree<HTML::Node>::iterator itNext;
        for (jt = it.begin(); jt != it.end();) {
            itNext = jt;
            itNext.skip_children();
            itNext++;
            optional<tree<HTML::Node>::iterator> optionalNext;
            if (itNext != it.end()) {
                optionalNext = itNext.begin();
            }
            if (Filter(dom, jt, optionalNext)) {
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
             tag == "ul") && // || tag == "p") &&
                it.number_of_children() >= 1)
        {
            if (!HasInterestingContent(it, tag)) {
                return true;
            }
        }

        string nextTag;
        if (tag == "p" && it.number_of_children() >= 1) {
            if (next.is_initialized()) {
                nextTag = GetTag(*next);
            }
            if (nextTag == "p" && next->number_of_children() >= 1) {
                if ((!HasInterestingContent(it, tag) &&
                        !HasInterestingContent(*next, nextTag)) ||
                        !HasInterestingContent(it, tag, false))
                {
                    return true;
                }
            } else {
                if (!HasInterestingContent(it, tag)) {
                    return true;
                }
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

void GetTitles(tree<HTML::Node>::iterator it, vector<string>& titles) {
    HTML::Node& node = *it;
    if (node.isTag()) {
        string tag = GetTag(it);
        if (tag == "title") {
            if (it.number_of_children() == 1) {
                titles.push_back(it.begin()->text());
                return;
            }
        } else if (tag == "meta") {
            node.parseAttributes();
            map<string, string> attributes = node.attributes();
            map<string, string>::iterator it;
            bool hasTitle = false;
            string content;
            for (it = attributes.begin(); it != attributes.end(); it++) {
                string key = it->first;
                string value = it->second;
                boost::algorithm::to_lower(key);
                boost::algorithm::to_lower(value);
                if (key == "property" &&
                        (value == "og:title" ||
                         value == "twitter:title"))
                {
                    hasTitle = true;
                }
                if (key == "content") {
                    content = value;
                }
                if (hasTitle) {
                    titles.push_back(content);
                }
            }
        } else if (tag != "html" && !tag.empty()) {
            return;
        }

        for (tree<HTML::Node>::iterator jt = it.begin(); jt != it.end(); jt++) {
            GetTitles(jt, titles);
        }
    }
}

void GetDescription(tree<HTML::Node>::iterator it, string& description) {
    HTML::Node& node = *it;
    if (node.isTag()) {
        string tag = GetTag(it);
        if (tag == "meta") {
            node.parseAttributes();
            map<string, string> attributes = node.attributes();
            map<string, string>::iterator it;
            bool hasDescritpion = false;
            string content;
            for (it = attributes.begin(); it != attributes.end(); it++) {
                string key = it->first;
                string value = it->second;
                boost::algorithm::to_lower(key);
                boost::algorithm::to_lower(value);
                if ((key == "property" ||
                     key == "name") &&
                        (value == "og:description" ||
                         value == "twitter:description" ||
                         value == "description"))
                {
                    hasDescritpion = true;
                }
                if (key == "content") {
                    content = value;
                }
                if (hasDescritpion) {
                    description = DecodeHtmlEntities(content);
                }
            }
        } else if (tag != "html" && !tag.empty()) {
            return;
        }

        for (tree<HTML::Node>::iterator jt = it.begin(); jt != it.end(); jt++) {
            GetDescription(jt, description);
        }
    }
}

// search for titles in dom tree, select a minimal one
void GetTitle(tree<HTML::Node>::iterator it, string& title, vector<string>& titles) {
    GetTitles(it, titles);

    if (titles.empty()) {
        return;
    }
    size_t minTitle = 0;
    for (size_t i = 1; i < titles.size(); i++) {
        if (titles[i].size() < titles[minTitle].size() &&
                titles[i].size() > 0 &&
                !islower(titles[i][0]))
        {
            minTitle = i;
        }
    }
    title = titles[minTitle];
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

string GetBasePath(string path) {
    boost::trim_if(path, boost::is_any_of("/"));
    vector<string> pathParts;
    boost::algorithm::split(pathParts, path, boost::algorithm::is_any_of("/"));
    int i = (int)pathParts.size() - 1;
    for (; i > 0; i--) {
        if (!(boost::algorithm::starts_with(pathParts[i], "b") ||
            boost::algorithm::starts_with(pathParts[i], "i") ||
            boost::algorithm::starts_with(pathParts[i], "ol") ||
            boost::algorithm::starts_with(pathParts[i], "li") ||
            boost::algorithm::starts_with(pathParts[i], "strong") ||
            boost::algorithm::starts_with(pathParts[i], "p")))
        {
            break;
        }
    }
    string result = "/";
    for (size_t j = 0; j <= i; j++) {
        result += pathParts[j] + "/";
    }
    return result;
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
        boost::algorithm::trim(element.Text);
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
        TElements::const_iterator next = it;
        next++;

        bool skip = false;
        string currentText = it->Text;

        if (TextFiltered(currentText) && (next == elements.end() ||
                                          next->Path != it->Path))
        {
            continue;
        }

        size_t wordsCount = CalcWordsCount(currentText);
        if (wordsCount > 3 && title.find(currentText) != string::npos) {
            it->Type |= TP_Header;
        }
        if (CalcSentencesCount(currentText) > 1) {
            if (!(next != elements.end() && wordsCount > 4 &&
                    next->Text.find(currentText) != string::npos)) {
                it->Type &= ~TP_Header;
            }
        }
        currentText = DecodeHtmlEntities(currentText);
        float distance = numeric_limits<float>::max();

        if (next != elements.end()) {
            distance = GetPathDistance(it->Path, next->Path);
        }

        if (distance > 1.6) {
            currentText = ImproveText(currentText);
        }

        bool makeNewBlock = false;
        makeNewBlock |= GetPathDistance(prevPath, it->Path) >= 5;

        makeNewBlock |= (it->Type & TP_Header &&
                        next != elements.end() &&
                        next->Path != prevPath);

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

        if (current.Text.empty() && current.Title.empty()) {
            // skips links at the begining of block
            if (it->Type & TP_Link) {
                skip = true;
            }
        }

        if ((it->Type & TP_Text) &&
                (!makeNewBlock || !(it->Type & TP_Header)) &&
                !skip)
        {
            if (boost::algorithm::starts_with(currentText, "'ın") &&
                    !current.Text.empty() && current.Text[current.Text.size() - 1] == ' ')
            {
                current.Text.erase(current.Text.size() - 1);
            }
            current.Text += currentText + " "; // todo: add dot if required
            current.Path = GetCommonPath(current.Path, it->Path);
            prevPath = it->Path;
        }
        if (it->Type & TP_Link) {
            current.Links.push_back(currentText);
        }
        if (it->Type & TP_Header) {
            current.Headers.push_back(currentText);
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

bool SimmilarTitle(const string& title, const vector<string>& titles) {
    if (title.empty()) {
        return false;
    }
    for (size_t i = 0; i < titles.size(); i++) {
        if (title.find(titles[i]) != string::npos ||
                titles[i].find(title) != string::npos)
        {
            return true;
        }
    }
    return false;
}

bool SimmilarDescription(const string& text, const string& normalizedDescription) {
    return NormalizeText(text).find(normalizedDescription);
}

bool SameTexts(const string& text1, const string& text2) {
    return NormalizeText(text1).find(NormalizeText(text2)) != string::npos;
}

bool IsGoodText(const string& text, size_t links, size_t headers) {
    size_t wordsCount = CalcWordsCount(text);
    if (((float)links / (float)wordsCount > 0.1) ||
            ((headers > 1) && ((float)headers / (float)wordsCount > 0.02)))
    {
        return false;
    }
    return true;
}

void PrepareBlocks2(vector<TContentBlock>& blocks,
                   const vector<string>& titles,
                   const string& mainTitle,
                   const string& description)
{
    size_t maxBlock = (size_t)-1;
    for (size_t i = 0; i < blocks.size(); i++) {
        if (( maxBlock == (size_t)-1 ||
              blocks[i].Text.size() > blocks[maxBlock].Text.size()) &&
                IsGoodText(blocks[i].Text,
                           blocks[i].Links.size(),
                           blocks[i].Headers.size()))
        {
            maxBlock = i;
        }
    }

    if (maxBlock == (size_t)-1) {
        maxBlock = 0;
        for (size_t i = 1; i < blocks.size(); i++) {
            if (blocks[i].Text.size() > blocks[maxBlock].Text.size()) {
                maxBlock = i;
            }
        }
    }

    blocks[maxBlock].Type = BT_MainContent;

    string normalizedDescr = NormalizeText(description);
    normalizedDescr = normalizedDescr.substr(normalizedDescr.size() * 0.1,
                            normalizedDescr.size() * 0.8);

    bool totalMerge = CalcWordsCount(blocks[maxBlock].Text) < 55;

    for (size_t i = maxBlock - 1; i != (size_t)-1; i--) {
        if (((SimmilarTitle(blocks[i].Title, titles) ||
             SimmilarDescription(blocks[i].Text, normalizedDescr) &&
             !SameTexts(blocks[maxBlock].Text, blocks[i].Text)) &&
                IsGoodText(blocks[i].Text,
                           blocks[i].Links.size(),
                           blocks[i].Headers.size())) ||
                totalMerge ||
                GetBasePath(blocks[i].Path) == GetBasePath(blocks[maxBlock].Path))
        {
            totalMerge = true;
            blocks[maxBlock].Text = blocks[i].Text + " " + blocks[maxBlock].Text;
        }
    }

    for (size_t i = maxBlock + 1; i < blocks.size(); i++) {
        if (GetBasePath(blocks[i].Path) == GetBasePath(blocks[maxBlock].Path)) {
            blocks[maxBlock].Text += blocks[i].Text + " " ;
        }
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
    return blocks[0];
}


vector<TContentBlock> ExhausteContent(const string& htmlData,
                               const TSettings& settings)
{
    HTML::ParserDom parser;
    tree<HTML::Node> dom = parser.parseTree(htmlData);

    string charset = settings.Charset;
    string title;   // main title
    string description; // description, parsed from meta
    vector<string> titles; // all detected titles, including og-meta

    GetCharset(dom.begin(), charset);
    GetTitle(dom.begin(), title, titles);
    GetDescription(dom.begin(), description);

    Filter(dom, dom.begin(), optional<tree<HTML::Node>::iterator>());
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

    PrepareBlocks2(blocks, titles, title, description);

    if (settings.DebugOutput) {
        DumpBlocks(blocks, *settings.DebugOutput);
    }

    assert(!blocks.empty() && "no blocks returned");
    return blocks;
}

} // NExhauster
