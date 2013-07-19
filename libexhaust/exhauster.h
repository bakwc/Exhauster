#pragma once
#include <string>

#include <contrib/htmlcxx/html/ParserDom.h>

#include "types.h"

using namespace std;
using namespace htmlcxx;

namespace NExhauster {

// public interface

TContentBlock ExhausteMainContent(const string& htmlData,
                           const TSettings& settings = TSettings());

vector<TContentBlock> ExhausteContent(const string& htmlData,
                               const TSettings& settings = TSettings());


// internal functions

bool CanBeDate(const string& word);
bool TextFiltered(const string& text);
bool ClassFiltered(const string& name);

string GetTag(tree<HTML::Node>::iterator it);

bool HasInterestingContent(tree<HTML::Node>::iterator it,
                           string parentTag = "",
                           bool hard = true,
                           size_t* words = NULL);

void AddDistance(const string& path, size_t from, float& distance);
float GetPathDistance(const string& path1, const string& path2);
string GetCommonPath(const string& path1, const string& path2);


// builds element list from dom tree
void MakeElements(TElements& elements,
                  const tree<HTML::Node>& dom,
                  string path = "",
                  unsigned char elementType = TP_Text);

void TrimHeaderElements(TElements& elements);

void MakeBlocks(vector<TContentBlock>& blocks, const TElements& elements);

} // NExhauster
