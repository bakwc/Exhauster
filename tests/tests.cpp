// tests.cpp

#include <boost/filesystem/operations.hpp>
#include <gtest/gtest.h>

#include <utils/string.h>
#include <libexhaust/exhauster.h>
#include <server/server.h>

using namespace boost::filesystem3;

TEST(utils, CalcWordsCount) {  
    ASSERT_EQ(0, CalcWordsCount(""));
    ASSERT_EQ(0, CalcWordsCount("   \n "));
    ASSERT_EQ(1, CalcWordsCount("    word2 \t"));
    ASSERT_EQ(3, CalcWordsCount("word1 word2 word3"));
    ASSERT_EQ(4, CalcWordsCount("word1;word2;word3 word4"));
    ASSERT_EQ(2, CalcWordsCount("тест кирилицы"));
    ASSERT_EQ(1, CalcWordsCount("test"));
    ASSERT_EQ(1, CalcWordsCount("2006"));
    ASSERT_EQ(3, CalcWordsCount("Tarih : 2012.11.09 17:25:20"));
}

TEST(utils, CalcSentencesCount) {
    ASSERT_EQ(0, CalcSentencesCount(""));
    ASSERT_EQ(1, CalcSentencesCount("word"));
    ASSERT_EQ(1, CalcSentencesCount("Some word"));
    ASSERT_EQ(1, CalcSentencesCount("Some real world."));
    ASSERT_EQ(2, CalcSentencesCount("That it two sentences text. And the second one."));
    ASSERT_EQ(1, CalcSentencesCount("Some multi-dot sentence..."));
    ASSERT_EQ(2, CalcSentencesCount("CFR Cluj’u deplasmanda 3-1 mağlup ederek Şampiyonlar Ligi H grubunda iddialı duruma gelen Galatasaray Braga ile kader maçına çıkacak. / Braga Galatasaray maçı tur organizasyonu için geç kalmayın…"));
}

TEST(utils, NormalizeText) {
    ASSERT_EQ("some text normalization", NormalizeText(" \nSome text\t \"normalization\"!!\n"));
    ASSERT_EQ("нормализация кирилицы тест", NormalizeText("<Нормализация> кирилицы\n\nТЕСТ!"));
    ASSERT_EQ("18:30 soft normalize.com", NormalizeText("18:30 Soft Normalize.Com", false));
    ASSERT_EQ("soft normalize com", NormalizeText("18:30 Soft Normalize.Com"));
}

TEST(utils, ImproveText) {
    ASSERT_EQ("Какой-то простой текст с кривым форматированием.",
              ImproveText("Какой-то простой    текст с\nкривым форматированием\n."));
    ASSERT_EQ("Ставим точку в конце предложения.",
              ImproveText("Ставим точку в конце предложения"));
}

TEST(utils, DecodeHtmlEntities) {
    ASSERT_EQ("hello world", DecodeHtmlEntities("hello&nbsp;world"));
}

TEST(server, TRequestGetParam) {
    NHttpServer::TRequest request;
    request.Query = "id=81&value=test";
    ASSERT_EQ("81", *request.GetParam("id"));
    ASSERT_EQ("test", *request.GetParam("value"));
}

TEST(libexhaust, GetPathDistance) {
    string path1 = "/html/body/div:class=fixed/div:class=inner/div:class=container/div:class=main/div:class=box_outer/article:class=cat_article/div:class=single_article_content;id=article_content/div:id=nrelate_flyout_placeholder/p/";
    string path2 = "/html/body/div:class=fixed/div:class=inner/div:class=container/div:class=main/div:class=box_outer;id=feature_outer/div:class=Feature_news/div:class=slider_wrap/div:class=slider_items/div:class=slider/div:class=slider_item/div:style=position:relative; overflow:hidden;/div:class=slider_caption/h2/atarget=_blank/";
    ASSERT_EQ(14.5, NExhauster::GetPathDistance(path1, path2));
}

struct TExhaustTestTask {
    string Name;
    string HtmlDataFile;
    string ContentFile;
};

TEST(libexhaust, TExhaustMainContentFunctional) {
    vector<TExhaustTestTask> tasks;
    directory_iterator it(TEST_DATA_DIR);
    directory_iterator end;
    while (it != end) {
        path p = it->path();
        if (p.extension().string() == ".txt") {
            TExhaustTestTask task;
            task.HtmlDataFile = TEST_DATA_DIR + p.stem().string() + ".html";
            task.ContentFile = TEST_DATA_DIR + p.stem().string() + ".txt";
            task.Name = p.stem().string();
            tasks.push_back(task);
        }
        it++;
    }

    for (size_t i = 0; i < tasks.size(); i++) {
        string htmlData = LoadFile(tasks[i].HtmlDataFile);
        string content = LoadFile(tasks[i].ContentFile);
        cout << "             checking '" << tasks[i].Name << "'\n";
        ASSERT_EQ(content, NExhauster::ExhausteMainContent(htmlData).Text);
    }
}

int main(int argc, char **argv) {
    setlocale(LC_CTYPE, "en_US.UTF-8");
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
