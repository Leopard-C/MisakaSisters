SELECT * INTO OUTFILE '/var/lib/mysql-files/misaka_sisters_info.csv'
 FIELDS TERMINATED BY ';' LINES TERMINATED BY '\n' FROM
 (SELECT 'mid','name','misaka_id','exact','level','sign','gender','face','fans','videos' union 
 SELECT `mid`,`name`,`misaka_id`,`exact`,`level`,`sign`,`gender`,`face`,`fans`,`videos`
 FROM MisakaSisters.misaka_sisters_info) b;

SELECT * INTO OUTFILE '/var/lib/mysql-files/nicknames.csv'
 FIELDS TERMINATED BY ';' LINES TERMINATED BY '\n' FROM
 (SELECT 'nickname','exist','misaka_id' union 
 SELECT `nickname`,`exist`,`misaka_id`
 FROM MisakaSisters.nicknames) b;
