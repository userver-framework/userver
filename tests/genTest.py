#!/usr/bin/python
# -*- coding: utf-8 -*-

import psycopg2
import random, string, sys
import numpy as np
from functools import partial
from itertools import chain

COUNT_OF_USERS = 30000
COUNT_OF_ARTICLES = 30000
COUNT_OF_COMMENTS = 15000
MAX_FOLLOWERS = 200 # Максимум Ффоловерок у каждого пользователя
MAX_FAVORITES = 200 # Максимум лайков у каждой статьи по отдельности
NAME_SCHEMA = "real_medium"

TITLES_DEFAULT = ["Monkey eat banan", "123456", "title", "article", "123"]
USERS_DEFAULT = ["vasya", "jake", "123", "root"]

TAG_LIST = ["c++", "cpp", "python", "ai", "guid", "backend", "frontend", "userver"]

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
    
    resList = []
    for i in range(COUNT_OF_USERS):
        resList.append(random.choice(nameList) + "_" + str(i))

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
        text += random.choice(words).capitalize()
        cnt_words =  random.randint(2, 6)
        for i in range(cnt_words):
            text += " " + (random.choice(words))
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

def progress_bar(current, total, bar_length=20):
    fraction = current / total

    arrow = int(fraction * bar_length - 1) * '-' + '>'
    padding = int(bar_length - len(arrow)) * ' '

    ending = '\n' if current == total else '\r'

    print(f'[{arrow}{padding}] {int(fraction*100)}%', end=ending)


def createStrExecuteDelete(nameTable):
    strExecute = "TRUNCATE " + NAME_SCHEMA + "." + nameTable + " CASCADE"
    return strExecute

def createStrExecuteInsert(nameTable, kwargs):
    strExecute = "INSERT INTO " + NAME_SCHEMA + "." + nameTable + " (" + ", ".join(kwargs.keys()) + ") VALUES(\'" + "\', \'".join(kwargs.values()) + "\')"
    return strExecute

def clearDataBase(con):
    print("Clear db")
    
    cur = con.cursor()

    tableList = ["favorites", "article_tag", "followers", "articles", "users", "tag_list"]
    for nameTable in tableList:
        cur.execute(createStrExecuteDelete(nameTable))

    print("End")

def fillUsers(con):
 

    cur = con.cursor()
   
    emails = genEmailList(COUNT_OF_USERS)
    lenUsersDefault = len(USERS_DEFAULT)
    usernames = USERS_DEFAULT + genUsername(max(0, COUNT_OF_USERS - lenUsersDefault)) 
    imageList = ["https://fikiwiki.com/uploads/posts/2022-02/1644991237_23-fikiwiki-com-p-kartinki-krasivikh-koshechek-31.jpg",
                 "https://webpulse.imgsmail.ru/imgpreview?key=pulse_cabinet-image-c52e6c20-071b-46fe-b0c0-cbad32ffb447&mb=webpulse",
                 "https://vplate.ru/images/article/orig/2019/04/belye-koshki-s-golubymi-glazami-harakterna-li-dlya-nih-gluhota-i-kakimi-oni-byvayut-29.jpg",
                 "https://limonos.ru/uploads/posts/2014-11/14148774331111.jpeg",
                 "https://proprikol.ru/wp-content/uploads/2020/11/kartinki-paczanov-43.jpg"]
    
    for i in range(COUNT_OF_USERS):
        progress_bar(i+1, COUNT_OF_USERS)
        randPassHash = "".join(random.choices(list(string.ascii_letters), k = 255))
        randBio = genText(15, 0)
        randImage = random.choice(imageList)

        if(i < lenUsersDefault):
            emails[i] = usernames[i] + "@yandex.ru"

        dictOfExecute = {"email":emails[i],
                         "username":usernames[i],
                         "password_hash":"8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92",
                         "bio":randBio,
                         "image":randImage}
        

        strExecute = createStrExecuteInsert("users", dictOfExecute)
        cur.execute(strExecute)




def fillArticles(con):
    
    cur = con.cursor()

    
    titles = TITLES_DEFAULT + [] 
    newLenght = max(0, COUNT_OF_ARTICLES - len(TITLES_DEFAULT))

    for i in range(newLenght):
        titles.append(genText(random.randint(10, 20), 1))
    
    userIDList = getColumn(con, "users", "user_id")

    for i in range(COUNT_OF_ARTICLES):
        progress_bar(i+1, COUNT_OF_ARTICLES)

        title = titles[i]
        slug = title.replace('.', "").replace(' ', '-').lower()
        slug += "_" + str(i)
        body = genText(random.randint(100, 400), 0)
        description = genText(20, 0)
        favorites_count = random.randint(0, COUNT_OF_USERS)
        userID = random.choice(userIDList)

        
        dictOfExecute = {"slug":slug,
                         "title":title,
                         "body":body,
                         "description":description,
                         "favorites_count":str(favorites_count),
                         "user_id":str(userID)}

        strExecute = createStrExecuteInsert("articles", dictOfExecute)
   
        cur.execute(strExecute)

def fillTAG_LIST(con):
    cur = con.cursor()

    cntTag = len(TAG_LIST)
    for i in range(cntTag):
        progress_bar(i+1, cntTag)
        strExecute = createStrExecuteInsert("tag_list", {"tag_name":TAG_LIST[i]})
        cur.execute(strExecute)   

def fillArticleTag(con):

    cur = con.cursor()
    

    articleIDList = getColumn(con, "articles", "article_id")
    tagIDList = getColumn(con, "tag_list", "tag_id")

    cnt = 1
    for articleID in articleIDList:
        progress_bar(cnt, COUNT_OF_ARTICLES)
        cnt +=1
        randTagIDList = random.sample(tagIDList, random.randint(1, len(tagIDList)))
        for tagID in randTagIDList:
            strExecute = createStrExecuteInsert("article_tag", {"article_id": articleID, "tag_id":tagID})
            cur.execute(strExecute) 



def fillFavorites(con):
    cur = con.cursor()

    articleIDList = getColumn(con, "articles", "article_id")
    userIDList = getColumn(con, "users", "user_id")

    cnt = 1
    curPos = 0

    for userID in userIDList:
        progress_bar(cnt, COUNT_OF_ARTICLES)
        
        cnt += 1
        
        step = random.randint(1, MAX_FAVORITES)
        
        if(curPos + step > COUNT_OF_ARTICLES):
            curPos = 0
        
        randArticleIDList = articleIDList[curPos:curPos+step]
        curPos+=step

        for articleID in randArticleIDList:
            strExecute = createStrExecuteInsert("favorites", {"user_id":userID, "article_id": articleID})
            cur.execute(strExecute)    

def fillFollowers(con):

    cur = con.cursor()

    userIDList = getColumn(con, "users", "user_id")

    cnt = 1
    curPos =0
    for followed in userIDList:
        progress_bar(cnt, COUNT_OF_USERS)
        cnt += 1

        step = random.randint(1, MAX_FOLLOWERS)
        
        if(curPos + step > COUNT_OF_USERS):
            curPos = 0
        
        randFollowerList = userIDList[curPos:curPos+step]
        curPos += step

        for follower in randFollowerList:
            if(followed == follower):
                continue
            strExecute = createStrExecuteInsert("followers", {"followed_user_id":followed, "follower_user_id": follower})
            cur.execute(strExecute)    

def fillComments(con):
    cur = con.cursor()

    userIDList = getColumn(con, "users", "user_id")
    articleIDList = getColumn(con, "articles", "article_id")



    for i in range(COUNT_OF_COMMENTS):
        progress_bar(i+1, COUNT_OF_COMMENTS)
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

        if(COUNT_OF_USERS < MAX_FAVORITES):
            print(f"\nERROR:\n\t COUNT_OF_USERS < MAX_FAVORITES: {COUNT_OF_USERS} < {MAX_FAVORITES}")
            exit(1)
        
        if(COUNT_OF_USERS < MAX_FOLLOWERS):
            print(f"\nERROR:\n\t COUNT_OF_USERS < MAX_FOLLOWERS: {COUNT_OF_USERS} < {MAX_FOLLOWERS}")
            exit(1)
        
        if(len(sys.argv) == 1):
            sys.argv.append("realmedium_db-1")

        conn_string = "host=localhost port=8081 dbname=\'" + sys.argv[1] + "\' user=user password=password"
        print ("Connecting to database\n    ->{}".format(conn_string))
        
        con = psycopg2.connect(conn_string)
        
        print(f"\nCount of USER: {COUNT_OF_USERS}")
        print(f"Count of ARTICLE: {COUNT_OF_ARTICLES}")
        print(f"Max follower: {MAX_FOLLOWERS}")
        print(f"Max favorite: {MAX_FAVORITES}")
        print(f"TAG: {TAG_LIST}")
        print('\nDefault titles: ' + str(TITLES_DEFAULT))
        print('Default users: ' + str(USERS_DEFAULT) + "\n\tpassword=123456\temail=name@yandex.ru\n")

        clearDataBase(con)      
        print("Fill tables: ")
        print("users: ")
        fillUsers(con)
        print("articles: ")
        fillArticles(con)
        print("tag_list: ")
        fillTAG_LIST(con)
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
