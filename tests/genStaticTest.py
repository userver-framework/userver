#!/usr/bin/python
# -*- coding: utf-8 -*-

import psycopg2
import random
import string
import sys
import numpy as np
from functools import partial
from itertools import chain

COUNT_OF_USERS = 25000
COUNT_OF_ARTICLES = 25000
COUNT_OF_COMMENTS = 25000
COUNT_FOLLOWERS = 20  # У каждого зера
COUNT_FAVORITES = 20  # У Каждой статьи
NAME_SCHEMA = "real_medium"

TAG_LIST = ["c++", "cpp", "python", "ai",
            "guid", "backend", "frontend", "userver"]


def getColumn(con, nameTable, nameField):
    cur = con.cursor()
    cur.execute("SELECT " + nameField + " FROM " +
                NAME_SCHEMA + "." + nameTable)

    return list(chain.from_iterable(cur.fetchall()))


def progress_bar(current, total, bar_length=20):
    fraction = current / total

    arrow = int(fraction * bar_length - 1) * '-' + '>'
    padding = int(bar_length - len(arrow)) * ' '

    ending = '\n' if current == total else '\r'

    print(f'[{arrow}{padding}] {int(fraction*100)}% ({current}/{total})', end=ending)


def createStrExecuteDelete(nameTable):
    strExecute = "TRUNCATE " + NAME_SCHEMA + "." + nameTable + " CASCADE"
    return strExecute


def createStrExecuteInsert(nameTable, kwargs):
    strExecute = "INSERT INTO " + NAME_SCHEMA + "." + nameTable + \
        " (" + ", ".join(kwargs.keys()) + ") VALUES(\'" + \
        "\', \'".join(kwargs.values()) + "\')"
    return strExecute


def clearDataBase(con):
    print("Clear db")

    cur = con.cursor()

    tableList = ["favorites", "article_tag",
                 "followers", "articles", "users", "tag_list"]
    for nameTable in tableList:
        cur.execute(createStrExecuteDelete(nameTable))

    print("End")


def fillUsers(con):

    cur = con.cursor()

    for i in range(COUNT_OF_USERS):
        progress_bar(i + 1, COUNT_OF_USERS)
        dictOfExecute = {"email": "user" + str(i) + "@yandex.ru",
                         "username": "user"+str(i),
                         "password_hash": "8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92",
                         "bio": "Юмор всегда срабатывает, когда нужно произвести впечатление на пользователей в Инстаграм. Границы между остроумием и юмором часто сливаются.",
                         "image": "https://proprikol.ru/wp-content/uploads/2020/11/kartinki-paczanov-43.jpg"}

        strExecute = createStrExecuteInsert("users", dictOfExecute)
        cur.execute(strExecute)


def fillArticles(con):

    cur = con.cursor()

    userIDList = getColumn(con, "users", "user_id")

    for i in range(COUNT_OF_ARTICLES):
        progress_bar(i+1, COUNT_OF_ARTICLES)

        dictOfExecute = {"slug": "name-of-the-article-number-" + str(i),
                         "title": "name of the article. Number " + str(i),
                         "body": "Это какаое-то тело для статьи, длинною в целою жизнь. Зачем ты это читаешь, тут не будет ничего полезного или интересного, весь этот текст - просто пыль.\n"*20,
                         "description": "Описание этой статьи ещё более бессмысленное, ведь он описывают ту беcсмыслицу, что будет в этой статье.",
                         "favorites_count": "100",
                         "user_id": userIDList[i % (COUNT_OF_USERS-1)]}

        strExecute = createStrExecuteInsert("articles", dictOfExecute)

        cur.execute(strExecute)


def filtTagLIst(con):
    cur = con.cursor()

    cntTag = len(TAG_LIST)
    for i in range(cntTag):
        progress_bar(i+1, cntTag)
        strExecute = createStrExecuteInsert(
            "tag_list", {"tag_name": TAG_LIST[i]})
        cur.execute(strExecute)


def fillArticleTag(con):

    cur = con.cursor()

    articleIDList = getColumn(con, "articles", "article_id")
    tagIDList = getColumn(con, "tag_list", "tag_id")

    cnt = 1
    for articleID in articleIDList:
        progress_bar(cnt, COUNT_OF_ARTICLES)
        cnt += 1
        for tagID in tagIDList:
            strExecute = createStrExecuteInsert(
                "article_tag", {"article_id": articleID, "tag_id": tagID})
            cur.execute(strExecute)


def fillFavorites(con):
    cur = con.cursor()

    articleIDList = getColumn(con, "articles", "article_id")
    userIDList = getColumn(con, "users", "user_id")

    cnt = 1
    curPos = 0

    for userID in userIDList:
        progress_bar(cnt, COUNT_OF_USERS)

        cnt += 1

        if(curPos + COUNT_FAVORITES > COUNT_OF_ARTICLES):
            curPos = 0

        randArticleIDList = articleIDList[curPos:curPos+COUNT_FAVORITES]
        curPos += COUNT_FAVORITES

        for articleID in randArticleIDList:
            strExecute = createStrExecuteInsert(
                "favorites", {"user_id": userID, "article_id": articleID})
            cur.execute(strExecute)


def fillFollowers(con):

    cur = con.cursor()

    userIDList = getColumn(con, "users", "user_id")

    cnt = 1
    curPos = 0
    for followed in userIDList:
        progress_bar(cnt, COUNT_OF_USERS)
        cnt += 1

        if(curPos + COUNT_FOLLOWERS > COUNT_OF_USERS):
            curPos = 0

        randFollowerList = userIDList[curPos:curPos+COUNT_FOLLOWERS]
        curPos += COUNT_FOLLOWERS

        for follower in randFollowerList:
            if(followed == follower):
                continue
            strExecute = createStrExecuteInsert(
                "followers", {"followed_user_id": followed, "follower_user_id": follower})
            cur.execute(strExecute)


def fillComments(con):
    cur = con.cursor()

    userIDList = getColumn(con, "users", "user_id")
    articleIDList = getColumn(con, "articles", "article_id")

    for i in range(COUNT_OF_COMMENTS):
        progress_bar(i+1, COUNT_OF_COMMENTS)

        dictOfExecute = {"body": "О да!! Я восхищён этой бессмыслицей в кубе. Это всё явно показывает бессмысленость смысла",
                         "user_id": userIDList[i % (COUNT_OF_USERS-1)],
                         "article_id": articleIDList[i % (COUNT_OF_ARTICLES-1)]}

        strExecute = createStrExecuteInsert("comments", dictOfExecute)
        cur.execute(strExecute)


def main():
    con = None

    try:

        if(COUNT_OF_USERS < COUNT_FAVORITES):
            print(
                f"\nERROR:\n\t COUNT_OF_USERS < COUNT_FAVORITES: {COUNT_OF_USERS} < {COUNT_FAVORITES}")
            exit(1)

        if(COUNT_OF_USERS < COUNT_FOLLOWERS):
            print(
                f"\nERROR:\n\t COUNT_OF_USERS < COUNT_FOLLOWERS: {COUNT_OF_USERS} < {COUNT_FOLLOWERS}")
            exit(1)

        if(len(sys.argv) == 1):
            sys.argv.append("realmedium_db-1")

        conn_string = "host=localhost port=8081 dbname=\'" + \
            sys.argv[1] + "\' user=user password=password"
        print("Connecting to database\n    ->{}".format(conn_string))

        con = psycopg2.connect(conn_string)

        print(f"\nCount of USER: {COUNT_OF_USERS}")
        print(f"Count of ARTICLE: {COUNT_OF_ARTICLES}")
        print(f"Max follower: {COUNT_FOLLOWERS}")
        print(f"Max favorite: {COUNT_FAVORITES}")
        print(f"TAG: {TAG_LIST}")

        clearDataBase(con)
        print("Fill tables ")
        print("users: ")
        fillUsers(con)
        print("articles: ")
        fillArticles(con)
        print("tag_list: ")
        filtTagLIst(con)
        print("article_tag: ")
        fillArticleTag(con)
        print("favorite: ")
        fillFavorites(con)
        print("followers: ")
        fillFollowers(con)
        print("comments: ")
        fillComments(con)

        con.commit()

        print("Successfully finished")

    except psycopg2.DatabaseError as e:
        print(f'Error {e}')
        sys.exit(1)

    finally:
        if con:
            con.close()


if __name__ == "__main__":
    main()
