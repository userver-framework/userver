#!/usr/bin/python
# -*- coding: utf-8 -*-

import psycopg2
import random, string, sys
import numpy as np
from functools import partial
from itertools import chain

COUNT_OF_USERS = 1000
COUNT_OF_ARTICLES = 1000
COUNT_OF_TAGS = 100
COUNT_OF_COMMENTS = 1000
MAX_FOLLOWERS = 30 # Максимум феворитов у каждой статьи
MAX_FAVORITES = 30 # Максимум подписчиков у каждого по-отдельности
NAME_SCHEMA = "real_medium"

def generate_random_number(min_value, max_value, step):
    num = random.randint(min_value // step, max_value // step) * step
    return num

def genListOfRandStr(amount_of_keys, lenght=0, _randint=np.random.randint):
    keys = set()
    pickchar = partial(
        np.random.choice,
        np.array(list(string.ascii_lowercase + string.digits)))
    lenght = max(2, lenght)
    if(lenght == 0):
        while len(keys) < amount_of_keys:
            keys |= {''.join([pickchar() for _ in range(_randint(12, 20))]) for _ in range(amount_of_keys - len(keys))}
    else:
        while len(keys) < amount_of_keys:
            keys |= {''.join([pickchar() for _ in range(_randint(lenght-2, lenght))]) for _ in range(amount_of_keys - len(keys))}
                             
    return list(keys)

def genUsername(cnt):
    nameList = ["Vasya", "Petya", "yandex_boy", "hacker", "Jhon", "Dimon", "Kostya", "Mike", "Jhohan", "Ninja", "GodOfCode", "McMisha"
                "Japanist", "Danial", "Anna", "Ilya", "Vadim", "Vika", "meganagibator", "zemlekop", "usman", "ImMoRTaL", "DeadPool2009",
                "imissher", "killer"]
    listCyfix = genListOfRandStr(cnt, 5)
    
    resList = []
    for i in listCyfix:
        resList.append(random.choice(nameList) + i)

    return resList
    
def genText(cnt, isEnglish):
    wordsRussia = ["привет", "спасибо", "да", "нет", "здравствуйте", "как", 
             "так", "это", "все", "он", "она", "я", "ты", "мы", "вы", 
             "они", "там", "здесь", "тоже", "очень", "много", "мало", 
             "большой", "маленький", "хороший", "плохой", "новый", 
             "старый", "другой", "свой", "ваш", "их", "кто", "что", 
             "где", "когда", "почему", "какой", "какая", "какие", 
             "какое", "который", "этот", "тот", "сей", "такой", "такая", 
             "такие", "такое", "один", "два", "три", "четыре", "пять", 
             "шесть", "семь", "восемь", "девять", "десять", "рука", 
             "нога", "голова", "лицо", "глаз", "нос", "рот", "ухо", 
             "лошадь", "собака", "кошка", "дом", "улица", "город", 
             "страна", "мама", "папа", "брат", "сестра", "сын", "дочь", 
             "бабушка", "дедушка", "хлеб", "вода", "чай", "кофе", 
             "мясо", "рыба", "овощи", "фрукты", "молоко", "сахар", 
             "соль", "масло", "школа", "университет", "книга", "ручка", "карандаш", "стол", "стул"]
    
    wordEnglish = ["the", "of", "and", "to", "a", "in", "for", "is", "on", "that", "by", "this", 
                "with", "it", "from", "as", "at", "an", "was", "have", "are", "were", "has", 
                "or", "be", "been", "had", "but", "not", "can", "will", "they", "do", "when", 
                "which", "you", "if", "we", "more", "there", "so", "up", "about", "out", "up", 
                "if", "into", "than", "just", "its", "these", "their", "some", "would", "could", 
                "went", "one", "say", "who", "what", "them", "how", "like", "than", "than", "think", 
                "when", "where", "know", "take", "him", "her", "get", "your", "way", "make", "see", 
                "go", "use", "see", "time", "day", "year", "good", "women", "men", "children", "such", 
                "back", "new", "long", "other", "any", "work", "first", "people", "way", "find", "give"]

    words = []
    if(isEnglish):
        words = wordEnglish
    else:
        words = wordsRussia

    text = ""
    while len(text) < cnt:
        text += random.choice(wordsRussia).capitalize()
        cnt_words =  random.randint(2, 6)
        for i in range(cnt_words):
            text += " " + (random.choice(wordsRussia))
            if(len(text) > cnt):
                break
        text += ". "
    
    text = text[:len(text)-1]

    return text

def genEmailList(cnt):
    domenList = ["gmail.com", "email.ru", "yandex.ru"]
    emails = genUsername(cnt)

    lenOfEmails = len(emails)
    for i in range(lenOfEmails):
        emails[i] += "@" + random.choice(domenList)

    return emails

def getColumn(con, nameTable, nameField):
    cur = con.cursor()
    cur.execute("SELECT " + nameField + " FROM " + NAME_SCHEMA + "." + nameTable)
    
    return list(chain.from_iterable(cur.fetchall()))

def createStrExecuteDelete(nameTable):
    strExecute = "DELETE FROM " + NAME_SCHEMA + "." + nameTable
    return strExecute

def createStrExecuteInsert(nameTable, kwargs):
    strExecute = "INSERT INTO " + NAME_SCHEMA + "." + nameTable + " (" + ", ".join(kwargs.keys()) + ") VALUES(\'" + "\', \'".join(kwargs.values()) + "\')"
    return strExecute

def clearDataBase(con):
    print("Clear db")
    
    cur = con.cursor()

    dbList = ["favorites", "article_tag", "followers", "articles", "users", "tag_list"]
    for nameDB in dbList:
        cur.execute(createStrExecuteDelete(nameDB))

def fillUsers(con):
    print("Fill users")

    cur = con.cursor()
   
    emails = genEmailList(COUNT_OF_USERS)
    usernames = genUsername(COUNT_OF_USERS)
    imageList = ["https://fikiwiki.com/uploads/posts/2022-02/1644991237_23-fikiwiki-com-p-kartinki-krasivikh-koshechek-31.jpg",
                 "https://webpulse.imgsmail.ru/imgpreview?key=pulse_cabinet-image-c52e6c20-071b-46fe-b0c0-cbad32ffb447&mb=webpulse",
                 "https://vplate.ru/images/article/orig/2019/04/belye-koshki-s-golubymi-glazami-harakterna-li-dlya-nih-gluhota-i-kakimi-oni-byvayut-29.jpg",
                 "https://limonos.ru/uploads/posts/2014-11/14148774331111.jpeg",
                 "https://proprikol.ru/wp-content/uploads/2020/11/kartinki-paczanov-43.jpg"]
    
    for i in range(COUNT_OF_USERS):
        randPassHash = "".join(random.choices(list(string.ascii_letters), k = 255))
        randBio = genText(15, 0)
        randImage = random.choice(imageList)

        dictOfExecute = {"email":emails[i],
                         "username":usernames[i],
                         "password_hash":randPassHash,
                         "bio":randBio,
                         "image":randImage}
        

        strExecute = createStrExecuteInsert("users", dictOfExecute)
        cur.execute(strExecute)

def fillArticles(con):
    print("Fill articles")
    
    cur = con.cursor()

    titles = []
    while len(titles) < COUNT_OF_ARTICLES:
        titles.append(genText(random.randint(10, 20), 0))

    userIDList = getColumn(con, "users", "user_id")

    for i in range(COUNT_OF_ARTICLES):
 
        title = titles[i]
        slug = title.replace('.', "").replace(' ', '-').lower()
        slug += "_" + genListOfRandStr(1, 5)[0]
        body = genText(60, 0)
        description = genText(20, 0)
        favorites_count = random.randint(0, 1000)
        userID = random.choice(userIDList)

        
        dictOfExecute = {"slug":slug,
                         "title":title,
                         "body":body,
                         "description":description,
                         "favorites_count":str(favorites_count),
                         "user_id":str(userID)}

        strExecute = createStrExecuteInsert("articles", dictOfExecute)
   
        cur.execute(strExecute)

def fillTagList(con):
    print("Fill tag_list")
    cur = con.cursor()
    
    tagList = ["c++", "python", "ai", "guid", "backend", "frontend", "userver"]

    for i in tagList:
        strExecute = createStrExecuteInsert("tag_list", {"tag_name":i})
        cur.execute(strExecute)   

def fillArticleTag(con):
    print("Fill article_tag")

    cur = con.cursor()
    cur.execute(createStrExecuteDelete("article_tag"))

    articleIDList = getColumn(con, "articles", "article_id")
    tagIDList = getColumn(con, "tag_list", "tag_id")


    for articleID in articleIDList:
        randTagIDList = random.sample(tagIDList, random.randint(1, len(tagIDList)))
        for tagID in randTagIDList:
            strExecute = createStrExecuteInsert("article_tag", {"article_id": articleID, "tag_id":tagID})
            cur.execute(strExecute)   

def fillFavorites(con):
    print("Fill favorites")

    cur = con.cursor()
    cur.execute(createStrExecuteDelete("article_tag"))

    articleIDList = getColumn(con, "articles", "article_id")
    userIDList = getColumn(con, "users", "user_id")

    for userID in userIDList:
        randArticleIDList = random.sample(articleIDList, random.randint(1, min(MAX_FAVORITES, COUNT_OF_ARTICLES)))
        for articleID in randArticleIDList:
            strExecute = createStrExecuteInsert("favorites", {"user_id":userID, "article_id": articleID})
            cur.execute(strExecute)    

def fillFollowers(con):
    print("Fill followers")

    cur = con.cursor()
    cur.execute(createStrExecuteDelete("article_tag"))

    userIDList = getColumn(con, "users", "user_id")

    for followed in userIDList:
        randFollowerList = random.sample(userIDList, random.randint(1, min(MAX_FOLLOWERS, COUNT_OF_USERS)))
        for follower in randFollowerList:
            if(followed == follower):
                continue
            strExecute = createStrExecuteInsert("followers", {"followed_user_id":followed, "follower_user_id": follower})
            cur.execute(strExecute)    

def fillComments(con):
    print("Fill comments")

    cur = con.cursor()
    cur.execute(createStrExecuteDelete("article_tag"))

    userIDList = getColumn(con, "users", "user_id")
    articleIDList = getColumn(con, "articles", "article_id")



    for i in range(COUNT_OF_COMMENTS):
        userID = random.choice(userIDList)
        articleID = random.choice(articleIDList)

        dictOfExecute = {"body":genText(random.randint(20, 60), 0),
                         "user_id":userID,
                         "article_id":articleID}

        strExecute = createStrExecuteInsert("comments", dictOfExecute)
        cur.execute(strExecute)    







def main():
    con = None

    try:
        conn_string = "host=localhost port=8081 dbname=\'" + sys.argv[1] + "\' user=user password=password"
        print ("Connecting to database\n    ->{}".format(conn_string))
        
        con = psycopg2.connect(conn_string)

        
        clearDataBase(con)      
        
        fillUsers(con)
        fillArticles(con)
        fillTagList(con)
        fillArticleTag(con)
        fillFavorites(con)
        fillFollowers(con)
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