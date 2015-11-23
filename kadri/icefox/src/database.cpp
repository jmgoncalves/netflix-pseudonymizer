/**
 * Copyright (C) 2006-2007 Benjamin C. Meyer (ben at meyerhome dot net)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Benjamin Meyer nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "database.h"
#include "movie.h"
#include "user.h"

#include <qfile.h>
#include <qdir.h>
#include <qdebug.h>

#include "../../ymddate.cpp"
#include "binarysearch.h"

#ifdef Q_OS_WIN
#include <winmmap.h>
#else
#include <sys/mman.h>
#endif

//	USERFILENAME is defined in database.h (so that it can be used in the main.cpp files)
//	MOVIESFILENAME is defined in database.h (so that it can be used in the main.cpp files)
//	DATESFILENAME is defined in database.h (so that it can be used in the main.cpp files)
//	PROBEFILENAME is defined in database.h (so that it can be used in probe.h)
//	QUALIFYINGFILENAME is defined in database.h (so that it can be used in the main.cpp files)

DataBase::DataBase(const QString &rootPath) :
        storedmovies(0),
        storedvotes(0),
        m_totalVotes(0),
        storedUsers(0),
        storedUsersIndex(0),
        m_totalUsers(0),
        moviesFile(0),
        votesFile(0),
        userFile(0),
        userIndexFile(0),
        m_rootPath(rootPath),
		set_movie_dates(false)
{
    if (m_rootPath.isEmpty()) {
        QDir dir = QDir::current();
        while (!dir.isRoot()) {
            if (dir.entryList(QStringList(), QDir::AllDirs).contains("training_set")) {
                m_rootPath = QDir::current().relativeFilePath(dir.path());
                break;
            }
            dir.cdUp();
        }
    }
}

DataBase::~DataBase()
{
    if (moviesFile != 0) {
        munmap(storedmovies, moviesFile->size());
        delete moviesFile;
    }
    if (votesFile != 0) {
        munmap(storedvotes, votesFile->size());
        delete votesFile;
    }
    if (userFile != 0) {
        munmap(storedUsers, userFile->size());
        delete userFile;
    }
    if (userIndexFile != 0) {
        munmap(storedUsersIndex, userIndexFile->size());
        delete userIndexFile;
    }
}

QString DataBase::rootPath() const
{
    return m_rootPath;
}

void DataBase::saveDatabase(const QVector<uint> &vector, const QString &fileName)
{
    QFile database(fileName);
    if (database.open(QFile::WriteOnly)) {
        for (int i = 0; i < vector.count(); ++i)
            database.write((char*)(&vector.at(i)), sizeof(uint));
    } else {
        qWarning() << "Warning: Unable to save database:" << fileName;
    }
}

void DataBase::saveDatabaseText(const QVector<uint> &vector, const QString &fileName)
{
    QFile database(fileName);
    if (database.open(QFile::WriteOnly)) {
        QTextStream out(&database);
        for (int i = 0; i < vector.count(); ++i) {
            out << vector.at(i) << " ";
        }
    } else {
        qWarning() << "Warning: Unable to save database in text:" << fileName;
    }
}

bool userlessthan(const int &s1, const int &s2)
{
    return DataBase::guser(s1) < DataBase::guser(s2);
}

bool userlessthan_DateStruct(DateStruct ds1, DateStruct ds2){
	return ds1.userid < ds2.userid;
}

void DataBase::generateMovieDatabase()
{
	if(set_movie_dates) qDebug() << "Generating " << DATESFILENAME << ".data...";
	else qDebug() << "Generating movie database..";
    QVector<uint> votesIndex;
    QVector<uint> votes;
	QVector<uint> movieDates;

	// jmgoncalves - dynamically get m_totalMovies
	QDir trainingSetDir = QDir(rootPath() + "/training_set/");
	m_totalMovies = trainingSetDir.count()-2; // need to remove . and ..
	qDebug() << "m_totalMovies set to " << m_totalMovies;

    votesIndex.resize(m_totalMovies); // hardcoded so totalMovies() doesn't need a branch
	votes.reserve(101922780);	//	QVector crashes if we don't reserve lots of space for some reason
	movieDates.reserve(101922780);	//	QVector crashes if we don't reserve lots of space for some reason
    qFill(votesIndex.begin(), votesIndex.end(), 0);
    int percentDone = 0;
    uint c = 0;
    votesIndex[0] = 0;

    for (int i = 1; i < votesIndex.size() + 1; ++i) {
        QString fileName = QString(rootPath() + "/training_set/mv_%1.txt").arg(i, 7, 10, QChar('0'));
        QFile data(fileName);
        QVector<int> currentVotes;
		QVector<DateStruct> currentMovieDates;
        if (data.open(QFile::ReadOnly)) {
            QTextStream in(&data);
            while (!in.atEnd()) {
                QString line = in.readLine();
                int x = line.lastIndexOf(",") - 1;
                QString date = line.right(line.length() - x - 2);
                uint vote = line.mid(x, 1).toInt();
                uint user = line.mid(0, line.lastIndexOf(",") - 2).toInt();
                if (x != -2 && user <= 0) {
                    qWarning() << "warning: found a user with id 0, file " << fileName;
                    continue;
                }

                if (x != -2) {
                    ++c;
					if(set_movie_dates){
						YMDDATE moviedate(date.toStdString());
						DateStruct dstruct;
						dstruct.unix_days = moviedate.unix_days();
						dstruct.userid = user;
						currentMovieDates.append(dstruct);
					}
					else{
						currentVotes.append((vote << 29) | user);
					}
                }
            }
        } else {
            qWarning() << fileName << "doesn't exist!";
            return;
        }
        qSort(currentVotes.begin(), currentVotes.end(), userlessthan);
		qSort(currentMovieDates.begin(), currentMovieDates.end(), userlessthan_DateStruct);
        for (int j = 0; j < currentVotes.size(); ++j) {
            votes.append(currentVotes.at(j));
        }
		for (int j = 0; j < currentMovieDates.size(); ++j)
            movieDates.append(currentMovieDates.at(j).unix_days << 16);
        if (i != m_totalMovies) {
            votesIndex[i] = c;
        } else {
            //qDebug() << i << c << votes.count();
        }
        int t = i / (m_totalMovies / 100);
        if (t != percentDone && t % 5 == 0) {
            percentDone = t;
            qDebug() << fileName << percentDone << "%" << i << c;
        }
    }
    qDebug() << "Generated movie database.  Saving...";
	if(set_movie_dates){
		qDebug() << "Saving " << rootPath() + "/" + DATESFILENAME + ".data" << " ...";
		saveDatabase(movieDates, rootPath() + "/" + DATESFILENAME + ".data");
		movieDates.clear();
		movieDates.reserve(0);
	}
	else{
		saveDatabase(votes, rootPath() + "/" + MOVIESFILENAME + ".data");
		saveDatabase(votesIndex, rootPath() + "/" + MOVIESFILENAME + ".index");
		saveDatabaseText(votesIndex, rootPath() + "/" + MOVIESFILENAME + ".index.txt");
	}
    votesIndex.clear();
    votes.clear();
    qDebug() << "Database cache files have been created.";
}

void DataBase::generateUserDatabase()
{
    if (!isLoaded())
        return;
    qDebug() << "Generating user database...";
    Movie movie(this);
    QMap<int, QVector<uint> > users;
    for (int i = 1; i <= totalMovies(); ++i) {
        movie.setId(i);
        for (uint j = 0; j < movie.votes(); ++j) {
            uint user = movie.user(j);
            uint score = movie.score(j);
            users[user].append((score << 29) | i);
        }

        // some debug stuff;
        int t = i / (totalMovies() / 100);
        if (i % (totalMovies() / 100) == 0 && t % 5 == 0)
            qDebug() << t << "%";
    }
    qDebug() << "finshed initial sorting, sorting and saving...";

    QMapIterator<int, QVector<uint> > i(users);
    QVector<uint> userIndex;
    QVector<uint> userConverter;

    QFile database(rootPath() + "/" + USERFILENAME + ".data");
    if (!database.open(QFile::WriteOnly)) {
        qWarning() << "Warning: Unable to save user database";
        return;
    }

    int c = 0;
    userConverter.append(c);

    while (i.hasNext()) {
        i.next();
        // sort and append each user's votes
        int user = i.key();
        QVector<uint> userVotes = i.value();
        users.remove(i.key());
        qSort(userVotes.begin(), userVotes.end(), userlessthan);

        userIndex.append(user);
        for (int j = 0; j < userVotes.count(); ++j) {
            uint next = userVotes[j];
            database.write((char*)(&next), sizeof(uint));
            ++c;
        }
        userConverter.append(c);
    }
    for (int i = 0; i < userConverter.count(); ++i)
        userIndex.append(userConverter[i]);
    saveDatabase(userIndex, rootPath() + "/" + USERFILENAME + ".index");
    //jmgonc
    saveDatabaseText(userIndex, rootPath() + "/" + USERFILENAME + ".index.txt");
    qDebug() << "Finished saving user database.";
}

bool load(QFile *file, uint **pointer)
{
    bool error = false;
    if (file->size() != 0
            && file->exists()
            && file->open(QFile::ReadOnly | QFile::Unbuffered)) {
        *pointer = (uint*)
                   mmap(0, file->size(), PROT_READ, MAP_SHARED,
                        file->handle(),
                        (off_t)0);
        if (*pointer == (uint*) - 1) {
            qWarning() << "mmap failed" << file->fileName();
            error = true;
        }
    } else {
        qWarning() << "unable to load database" << file->fileName();
        error = true;
    }
    return error;
}

bool DataBase::load()
{
    QString movieFileName = rootPath() + "/" + MOVIESFILENAME + ".index";
    moviesFile = new QFile(movieFileName);
	if (!QFile::exists(movieFileName))
        generateMovieDatabase();

    bool moviesFileError = ::load(moviesFile, &storedmovies);
    // Basic sanity check
    //if (!moviesFileError && (storedmovies[0] != 0 || (storedmovies[1] != 547 && storedmovies[1] != 524))) {
    if (!moviesFileError && (storedmovies[0] != 0)) { // removed votes from sanity check because votes were removed from training set
        qWarning() << "Movie database error, possibly corrupt.  Expected [0] to be 0, but it is:"
        << storedmovies[0] << "or expected [1] to be 547/524, but it is:" << storedmovies[1];
        munmap(storedmovies, moviesFile->size());
        moviesFileError = true;
    }
    if (moviesFileError) {
        delete moviesFile;
        moviesFile = 0;
        return false;
    }

    votesFile = new QFile(rootPath() + "/" + MOVIESFILENAME + ".data");
    bool votesFileError = ::load(votesFile, &storedvotes);
    m_totalVotes  = votesFile->size() / 4;
    m_totalMovies = moviesFile->size() / 4; // jmgoncalves
    // Basic sanity check
    Movie m(this, 1);
    // removed votes from sanity check because votes were removed from training set
//    if (!votesFileError && (m.votes() != 547 && m.votes() != 524)) {
//        qWarning() << "votes database error, needs updating or possibly corrupt.  Expect movie" << m.id() << "to have 547/524 votes, but it only has:" << m.votes();
//        munmap(storedvotes, votesFile->size());
//        votesFileError = true;
//    }
//    if (!votesFileError && m.findScore(1488844) != 3) {
//        qWarning() << "votes database error, needs updating or possibly corrupt.  Expect movie" << m.id() << "to have rank of 3:" << m.findScore(1488844) << "for user 1488844.";
//        munmap(storedvotes, votesFile->size());
//        votesFileError = true;
//    }

    int voteSum = 0;
    for (int i=1; i<=totalMovies(); i++) {
    	Movie mov(this, i);
    	for (int j=0; j<mov.votes(); j++)
    		voteSum += mov.score(j);
    }
    qDebug() << "Loaded movie database with " << totalMovies() << " movies, " << m_totalVotes << " votes and " << voteSum << " vote sum";

    if (votesFileError) {
        delete votesFile;
        votesFile = 0;
        return false;
    }



    QString userFileName = rootPath() + "/" + USERFILENAME + ".data";
    if (!QFile::exists(userFileName))
        generateUserDatabase();

    userFile = new QFile(userFileName);
    bool userFileError = ::load(userFile, &storedUsers);
    userIndexFile = new QFile(rootPath() + "/" + USERFILENAME + ".index");
    bool userIndexFileError = ::load(userIndexFile, &storedUsersIndex);
    if (userFileError || userIndexFileError) {
        delete userIndexFile;
        delete userFile;
        userIndexFile = 0;
        userFile = 0;
        return false;
    }
    m_totalUsers = (isLoaded()) ? userIndexFile->size() / 8 : 0;
    users.reserve(totalUsers());
    for (int i = 0; i < totalUsers(); ++i) {
        users.insert(storedUsersIndex[i], i);
    }
    // Basic sanity check that the database is ok and as we expect it to be
    User user(this, 6);
    //if ((user.votes() != 626 && user.votes() != 623) || user.movie(0) != 30) {
    if (false) { // removed votes from sanity check because votes were removed from training set
        qWarning() << "Expected " << m.id() << " movie(0) to be user 30, but it is actually:" << user.movie(0);
        qWarning() << "OR user database error, possibly corrupt.  Expected " << user.id() << "to have 626 votes, but it only has:" << user.votes();
        qWarning() << "Delete users.index and users.data and restart this app to regenerate updated files.";
        munmap(storedUsers, userFile->size());
        munmap(storedUsersIndex, userIndexFile->size());
        delete userIndexFile;
        delete userFile;
        userIndexFile = 0;
        userFile = 0;
        users.clear();
        return false;
    }

    voteSum = 0;
	for (int i=1; i<=totalUsers(); i++) {
		for (int j=0; j<user.votes(); j++)
			voteSum += user.score(j);
		//qDebug() << "u.id()=" << user.id() << "; u.votes()=" << user.votes() << "; voteSum=" << voteSum;
		user.next();
	}
    qDebug() << "Loaded user database with" << totalUsers() << "users and" << userFile->size() / 4 << "votes and" << voteSum << "vote sum";

    return true;
}

void DataBase::setUserDates(){
	fprintf(stderr, "Setting user dates...\n");
	datesFileName = rootPath() + "/" + DATESFILENAME + ".data";
	datesFile = new QFile(datesFileName);
	QVector<uint> combinedDates;
	combinedDates.reserve(101922780);	//	To prevent crashing TODO trouble here?
	Movie currentMovie(this);
	User currentUser(this);
	bool datesmaperror = ::load(datesFile, &datesmap);
	if(datesmaperror){
		qDebug() << "Error mapping dates data file. Please re-run. Exiting.";
		exit(0);
	}
	int index = 0;
	//currentUser.setId(6); // 6 is the first userId in the original dataset
	currentUser.setId(1);
	int changed = 0;
	int unchanged = 0;
	for (int i = 0; i < totalUsers(); ++i) {
		int count = currentUser.votes();
		for (int v = 0; v < currentUser.votes(); ++v) {
			int movie = currentUser.movie(v);
			int userindex = i+1;
			//	Check to see if the date has already been set
			if(currentUser.votedate(v)==0){
				uint unix_days = currentMovie.votedate(movie, findUserAtMovie(userindex, movie));
				//	The dates map file should contain dates sorted by movies shifted << 16, so use boolean or
				combinedDates.append(datesmap[index] | unix_days);
				changed++;
			}
			else unchanged++;
			index++;
		}
		currentUser.next();
	}
	munmap(datesmap, datesFile->size());
	saveDatabase(combinedDates, datesFileName);
	fprintf(stderr, "Done setting user dates. Changed %d dates, %d unchanged.\n", changed, unchanged);
}

void DataBase::generateProbeDates(){
	qDebug() << "Generating probe dates from probe.better.txt...";
	QFile probeFile(rootPath()+"/probe.better.txt");
	QVector<uint> probeDates;
	if (probeFile.open(QFile::ReadOnly)) {
		QTextStream in(&probeFile);
		while (!in.atEnd()) {
			QString line = in.readLine();
			int x = line.lastIndexOf(",") - 1;
			if (x != -2) {
				QString date = line.right(line.length() - x - 2);
				YMDDATE moviedate(date.toStdString());
				int votedate = moviedate.unix_days();
				probeDates.append(votedate);
				if(votedate==0){
					uint user = line.mid(0, line.lastIndexOf(",") - 2).toInt();
					printf("Error - invalid votedate of 0 for user %d\n", user);
				}
			}
		}
		saveDatabase(probeDates, rootPath() + "/" + PROBEFILENAME + ".dates.data");
		qDebug() << "Finished generating probe dates (" << probeDates.size() << ").";
	}
	else{
		qDebug() << "Error reading " << probeFile.fileName() << ". Exiting.";
		exit(0);
	}
}

void DataBase::generateQualDates(){
	qDebug() << "Generating qualifying dates...";
	QFile qualFile(rootPath()+"/qualifying.txt");
	QVector<uint> qualDates;
	if (qualFile.open(QFile::ReadOnly)) {
		QTextStream in(&qualFile);
		while (!in.atEnd()) {
			QString line = in.readLine();
			int x = line.lastIndexOf(",") - 1;
			if (x != -2) {
				QString date = line.right(line.length() - x - 2);
				YMDDATE moviedate(date.toStdString());
				uint votedate = moviedate.unix_days();
				qualDates.append(votedate);
			}
		}
		saveDatabase(qualDates, rootPath() + "/" + QUALIFYINGFILENAME + ".dates.data");
		qDebug() << "Finished generating qualifying dates.";
	}
	else{
		qDebug() << "Error reading " << qualFile.fileName() << ". Exiting.";
		exit(0);
	}
}

inline int DataBase::findUserAtMovie(int uindex, int mindex){
	int m = mindex - 1;
	int u = uindex - 1;
	int userid = storedUsersIndex[u];
	int start = storedmovies[m];
	int numvotes = 0;
	if(mindex<totalMovies()) numvotes = storedmovies[m+1] - storedmovies[m];
	else numvotes = totalVotes() - storedmovies[m];
	int end = start + numvotes - 1;
	int i = userBinarySearch(storedvotes, userid, start, end);
	if(guser(storedvotes[i])==userid){
		return i - storedmovies[m];
	}
	return -1;
}
