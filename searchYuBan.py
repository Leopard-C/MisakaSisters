#!/usr/bin/python3

import csv
import re
import requests
import time
import os
import sys
import codecs
from lxml import etree

uids = []
flag1 = False
flag2 = False
flag3 = False
time_start = time.time()

if not os.path.exists('./cache'):
    os.mkdir('./cache')

# 记录已经读取的用户的唯一id
# 防止重复
uidsFile = open('./cache/uids', 'a+')


def searchYuBan(keyword):
    results = []

    global time_start
    global flag1
    global flag2
    global flag3
    global uids

    # Bilibili return 50 pages at most (20 records in one page)
    for i in range(1, 51):          # page
        # 每隔30s暂停5s
        time_end = time.time()
        duration = time_end - time_start
        if duration > 300:
            print('Sleeping 30s')
            time.sleep(30)
            time_start = time_end
            flag1 = flag2 = flag3 = False
        elif duration > 210 and flag3 == False:
            print('Sleeping 30s')
            time.sleep(30)
            flag3 = True
        elif duration > 110 and flag2 == False:
            print('Sleeping 20s')
            time.sleep(20)
            flag2 = True
        elif duration > 60 and flag1 == False:
            print('Sleeping 10s')
            time.sleep(10)
            flag1 = True

        print(keyword, "  page=", i)

        url = "https://search.bilibili.com/upuser?keyword=" + keyword + "&page=" +  str(i)
        try:
            res = requests.get(url)
        except BaseException as e:
            print(e)
            sleep(60)
            res = requests.get(url)

        if res.text.find('没有相关数据') != -1:
            break
        if res.status_code != 200:
            res = requests.get(url)

        html = etree.HTML(res.text)
        uls = html.xpath('//*[@id="user-list"]/div[1]/ul')

        if len(uls) == 0:
            continue

        for li in range(1, len(uls[0]) + 1):
            ahrefXPath = '//*[@id="user-list"]/div[1]/ul/li[%d]/div[2]/div[1]/a[1]/@href' %(li)
            atitleXPath = '//*[@id="user-list"]/div[1]/ul/li[%d]/div[2]/div[1]/a[1]/@title' %(li)
            ahrefs = html.xpath(ahrefXPath)
            atitles = html.xpath(atitleXPath)

            if len(ahrefs) == 0 or len(atitles) == 0:
                continue

            ahref = ahrefs[0]
            atitle = atitles[0]

            uname = atitle
            uid = ahref[ahref.find('com/')+4 : ahref.find('?')]

            # 是否重复
            if uid in uids:
                print("repeat: ", uid)
                continue
            else:
                uids.append(uid)

            uurl = 'https://space.bilibili.com/' + uid
            sisterId = re.findall('\d+', uname);

            if sisterId:
                row = [uurl, uid, uname]
                for digit in sisterId:
                    row.append(int(digit))
                print(row)
                results.append(row)

        # 本页用户数不为20，说明已到最后一页
        if len(uls[0]) != 20:
            print('break')
            break

    return results


def searchYuBanByIDs(startId, endId):
    results = []
    for i in range(startId, endId):
        keyword = '%E5%BE%A1%E5%9D%82%20' + str(i)
        resultTmp = searchYuBan(keyword)
        if resultTmp:
            results.extend(resultTmp)
    return results


def saveToFile(results, fileName):
    fileHeader = ["UserSpaceUrl", "UserId", "UserFullName", "sisterID..."]
    csvFile = open(fileName, "w")
    csvFile = codecs.open(fileName, 'w', encoding='utf-8')
    writer = csv.writer(csvFile)
    writer.writerow(fileHeader)
    for record in results:
        writer.writerow(record)
    csvFile.close()


def readUidsFile():
    global uidsFile
    uidsFile.seek(0)
    length = uidsFile.tell()
    lines = uidsFile.readlines()
    for i in range(0, len(lines)):
        uids.append(lines[i].rstrip('\n'))
    uidsFile.close()
    uidsFile = open('./cache/uids', 'w')

    print(uids)

def mergeFiles():
    root_path = './cache'
    outputFileName = 'YubanSisters.csv'
    outputFile = codecs.open(outputFileName, 'w', encoding='utf-8')
    fileHeader = ["UserSpaceUrl", "UserId", "UserFullName", "sisterID..."]
    writer = csv.writer(outputFile)
    writer.writerow(fileHeader)
    for i in os.walk(root_path):
        for j in i[2]:
            path = os.path.join(i[0], j)
            csvFile = codecs.open(path, 'r', encoding='utf-8')
            reader = csv.reader(csvFile)
            for iRow,row in enumerate(reader):
                if iRow > 0 and len(row) > 2:  # Read from line 2 and except file 'uids'
                    writer.writerow(row)
            csvFile.close()
    outputFile.close()


if __name__ == '__main__':
    try:
        time_start = time.time()

        readUidsFile()

        startId = 0
        endId = 300001
        step = 300

        # Input endId form command line
        # default value is 30001
        #
        if len(sys.argv) > 1:
            endId = int(sys.argv[1])

        # Search by id
        # '御坂 0' ~ '御坂 30000'
        #
        for i in range(startId, endId, step):
            start = i
            end = i + step
            if end > endId:
                end = endId
            results = searchYuBanByIDs(start, end)
            if results:
                fileName = './cache/' + str(start) + '-' + str(end)
                saveToFile(results, fileName)

        # Blur search (by keywords)
        keywords = [
            '%E5%BE%A1%E5%9D%82',                       # 御坂
            '%E5%BE%A1%E5%9D%82%E5%BE%A1%E5%9D%82',     # 御坂御坂
            '%E5%BE%A1%E5%9D%82%20%E5%8F%B7',           # 御坂 号
            '%E5%BE%A1%E5%9D%82%E5%A6%B9%E5%A6%B9',     # 御坂妹妹
            '%E5%BE%A1%E5%9D%82%2000',                  # 御坂 00 (%20代表空格)
            '%E5%BE%A1%E5%9D%82%20000',                 # 御坂 000
            '%E5%BE%A1%E5%9D%82%200000',                # 御坂 0000
            '%E5%BE%A1%E5%9D%82%2000000',               # 御坂 00000
            '%E5%BE%A1%E5%9D%82%20o',                   # 御坂 o (用字母o代表数字0)
            '%E5%BE%A1%E5%9D%82%20oo',                  # 御坂 oo
            '%E5%BE%A1%E5%9D%82%20ooo',                 # 御坂 ooo
            '%E5%BE%A1%E5%9D%82%20oooo',                # 御坂 oooo
            '%E5%BE%A1%E5%9D%82%20ooooo',               # 御坂 ooooo
            '%E5%BE%A1%E6%9D%BF',                       # 御板 (错别字)
            'yuban',                                    # (拼音)
            'みさか'                                    # (日语)
        ]
        for i in range(0, len(keywords)):
            results = searchYuBan(keywords[i])
            if results:
                fileName = './cache/' + str(i)
                saveToFile(results, fileName)

        print("OK!")

    except Exception as e:
        print(e)
    finally:
        for uid in uids:
            uidsFile.write(uid)
            uidsFile.write('\n')
        uidsFile.close()

        mergeFiles()

        print("End")


