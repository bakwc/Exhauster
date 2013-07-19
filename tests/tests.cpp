// tests.cpp

#include <utils/string.h>
#include <libexhaust/exhauster.h>

#include <gtest/gtest.h>

TEST(utils, CalcWordsCount) {
    ASSERT_EQ(0, CalcWordsCount(""));
    ASSERT_EQ(0, CalcWordsCount("   \n "));
    ASSERT_EQ(1, CalcWordsCount("    word2 \t"));
    ASSERT_EQ(3, CalcWordsCount("word1 word2 word3"));
    ASSERT_EQ(4, CalcWordsCount("word1;word2;word3 word4"));
    ASSERT_EQ(2, CalcWordsCount("тест кирилицы"));
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

int main(int argc, char **argv) {
    setlocale(LC_CTYPE, "en_US.UTF-8");
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
