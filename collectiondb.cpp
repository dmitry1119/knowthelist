/*
    Copyright (c) 2004 Mark Kretschmann <markey@web.de>
    Copyright (c) 2004 Christian Muehlhaeuser <chris@chris.de>
    Copyright (C) 2005-2014 Mario Stephan <mstephan@shared-files.de>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "collectiondb.h"
#include <QtSql>

#include <QDesktopServices>
#include <qimage.h>
#include <QCustomEvent>

class CollectionDbPrivate
{
    public:

        uint genreCount;
        QString lastPath;
        QString lastArtist;
        QString lastGenre;
        QString lastFilterString;
        QString filterString;
        ulong resultCount;
        ulong resultLength;
        QString quickFilter;
        QSqlDatabase *db;
        QSqlQuery *query;


        QString selectionFilter(QString year="", QString genre="", QString artist="", QString album="")
        {
            QString ret = "";
            if ( !year.isEmpty() )
                ret += "AND year.name = '" + year.replace( "'", "''" ) + "' ";
            if ( !genre.isEmpty() )
                ret += "AND genre.name = '" + genre.replace( "'", "''" ) + "' ";
            if ( !artist.isEmpty() )
                ret += "AND artist.name = '" + artist.replace( "'", "''" ) + "' ";
            if ( !album.isEmpty() )
                ret += "AND album.name = '" + album.replace( "'", "''" ) + "' ";
            return ret;
        }

        QString selectionFilterForRandom(QString path="", QString genre="", QString artist="")
        {
            QString ret = "";
            if ( !path.isEmpty() )
              ret += "AND lower(tags.url) like lower('%" + path.replace( "'", "''" ) + "%') ";
            if ( !genre.isEmpty() )
              ret += "AND lower(genre.name) like lower('%" + genre.replace( "'", "''" ) + "%') ";
            if ( !artist.isEmpty() )
              ret += "AND lower(artist.name) like lower('%" + artist.replace( "'", "''" ) + "%') ";
            return ret;
        }

};

CollectionDB::CollectionDB()
{
    p = new CollectionDbPrivate;
    db = QSqlDatabase::database();

    p->db = &db;
    p->query = new QSqlQuery(*(p->db));


    p->genreCount=0;

    p->quickFilter = "FROM tags, artist, album, year, genre WHERE 1=1 "
                " AND tags.artist = artist.id "
                " AND tags.album = album.id "
                " AND tags.year = year.id "
                " AND tags.genre = genre.id ";
}


CollectionDB::~CollectionDB()
{
    db.close();
    delete p;
    p=0;
}

QString
CollectionDB::escapeString( QString string )
{
    string.replace( "'", "''" );
    return string;
}

void CollectionDB::setFilterString( QString string )
{
    p->quickFilter = "FROM tags, artist, album, year, genre WHERE 1=1 "
                " AND tags.artist = artist.id "
                " AND tags.album = album.id "
                " AND tags.year = year.id "
                " AND tags.genre = genre.id ";

    if ( string != "" ) {
      string = escapeString( string );
      p->quickFilter += " AND ( artist.name LIKE '%" + string + "%' OR "
                    + "album.name LIKE '%" + string + "%' OR "
                    + "tags.title LIKE '%" + string + "%' OR "
                    + "lower(tags.url) LIKE '%" + string + "%' )";
    }
    p->filterString = string;
}

bool CollectionDB::isDbValid()
{
    if ( ( !executeSql( "SELECT COUNT( url ) FROM tags;" ) ) || ( !executeSql( "SELECT COUNT( url ) FROM statistics;" ) ) )
        return false;
    else
        return true;
}


bool CollectionDB::isEmpty()
{
    return ( selectSqlNumber("SELECT COUNT( url ) FROM tags;" ) < 1 );
}


void CollectionDB::incSongCounter( const QString url )
{
    QList<QStringList> entries;

    entries = selectSql( QString( "SELECT playcounter, createdate FROM statistics WHERE url = '%1';" )
                  .arg( escapeString( url ) ) );

    if ( !entries.isEmpty() )
    {
        // entry exists, increment playcounter and update accesstime
        executeSql( QString( "REPLACE INTO statistics ( url, createdate, accessdate, playcounter ) VALUES ( '%1', '%2', strftime('%s', 'now'), %3 );" )
                .arg( escapeString( url ) )
                .arg( entries.at(0)[1] )
                .arg( entries.at(0)[0] + " + 1" ) );
    } else
    {
        // entry didnt exist yet, create a new one
        executeSql( QString( "INSERT INTO statistics ( url, createdate, accessdate, playcounter ) VALUES ( '%1', strftime('%s', 'now'), strftime('%s', 'now'), 1 );" )
                .arg( escapeString( url ) ) );
    }
}


void CollectionDB::updateDirStats( QString path, const long datetime )
{
    if ( path.endsWith( "/" ) )
        path = path.left( path.length() - 1 );

    executeSql( QString( "REPLACE INTO directories ( dir, changedate ) VALUES ( '%1', %2 );" )
             .arg( escapeString( path ) )
             .arg( datetime ) );
}


void CollectionDB::removeSongsInDir( QString path )
{
    if ( path.endsWith( "/" ) )
        path = path.left( path.length() - 1 );

    executeSql( QString( "DELETE FROM tags WHERE dir = '%1';" )
             .arg( escapeString( path ) ) );
}


bool CollectionDB::isDirInCollection( QString path )
{
    if ( path.endsWith( "/" ) )
        path = path.left( path.length() - 1 );

   QList<QStringList> entries =  selectSql( QString( "SELECT changedate FROM directories WHERE dir = '%1';" )
             .arg( escapeString( path ) ) );

    return !entries.isEmpty();
}


void CollectionDB::removeDirFromCollection( QString path )
{
    if ( path.endsWith( "/" ) )
        path = path.left( path.length() - 1 );

    executeSql( QString( "DELETE FROM directories WHERE dir = '%1';" )
             .arg( escapeString( path ) ) );
}

long CollectionDB::selectSqlNumber( const QString& statement )
{
    if (p->query->exec(statement)) {
        while (p->query->next()) {
            return p->query->value(0).toInt();
        }
    }
    else
        qDebug() << p->query->lastError();
    return -1;
}


bool CollectionDB::executeSql( const QString& statement )
{
    if (p->query->exec(statement))
        return true;
    else {
       qDebug() << p->query->lastError();
       qDebug() << "Statement: " <<  statement;
       return false;
    }
}

QList<QStringList> CollectionDB::selectSql( const QString& statement)
{
    qDebug() << __FUNCTION__<< " SQL-query: " << statement;

    QList<QStringList> tags;
    tags.clear();
    int count;

    if (p->query->exec(statement)) {
        while (p->query->next()) {
            QStringList tag;
            count = p->query->record().count();
            for ( int i = 0; i < count; i++ ) {
                tag <<  p->query->value(i).toString() ;
            }
            tags << tag;
        }
    }
    else
       qDebug() << p->db->lastError() << "\n" << p->query->lastError();

    return tags;
}

void
CollectionDB::createTables( bool temporary )
{
    qDebug() << __FUNCTION__;

    //create tag table
    executeSql( QString( "CREATE %1 TABLE tags%2 ("
                        "%3"
                        "url VARCHAR(100),"
                        "dir VARCHAR(100),"
                        "artist INTEGER,"
                        "title VARCHAR(100),"
                        "album INTEGER,"
                        "genre INTEGER,"
                        "year INTEGER,"
                        "length INTEGER,"
                        "track NUMBER(4) );" )
                        .arg( temporary ? "TEMPORARY" : "" )
                        .arg( temporary ? "_temp" : "" ) 
                        .arg( temporary ? "id INTEGER," : "id INTEGER PRIMARY KEY," ));

    //create album table
    executeSql( QString( "CREATE %1 TABLE album%2 ("
                        "id INTEGER PRIMARY KEY,"
                        "name VARCHAR(100) );" )
                        .arg( temporary ? "TEMPORARY" : "" )
                        .arg( temporary ? "_temp" : "" ) );

    //create artist table
    executeSql( QString( "CREATE %1 TABLE artist%2 ("
                        "id INTEGER PRIMARY KEY,"
                        "name VARCHAR(100) );" )
                        .arg( temporary ? "TEMPORARY" : "" )
                        .arg( temporary ? "_temp" : "" ) );

    //create genre table
    executeSql( QString( "CREATE %1 TABLE genre%2 ("
                        "id INTEGER PRIMARY KEY,"
                        "name VARCHAR(100) );" )
                        .arg( temporary ? "TEMPORARY" : "" )
                        .arg( temporary ? "_temp" : "" ) );

    //create year table
    executeSql( QString( "CREATE %1 TABLE year%2 ("
                        "id INTEGER PRIMARY KEY,"
                        "name VARCHAR(100) );" )
                        .arg( temporary ? "TEMPORARY" : "" )
                        .arg( temporary ? "_temp" : "" ) );

    //create indexes
    executeSql( QString( "CREATE INDEX album_idx%1 ON album%2( name );" )
                .arg( temporary ? "_temp" : "" ).arg( temporary ? "_temp" : "" ) );
    executeSql( QString( "CREATE INDEX artist_idx%1 ON artist%2( name );" )
                .arg( temporary ? "_temp" : "" ).arg( temporary ? "_temp" : "" ) );
    executeSql( QString( "CREATE INDEX genre_idx%1 ON genre%2( name );" )
                .arg( temporary ? "_temp" : "" ).arg( temporary ? "_temp" : "" ) );
    executeSql( QString( "CREATE INDEX year_idx%1 ON year%2( name );" )
                .arg( temporary ? "_temp" : "" ).arg( temporary ? "_temp" : "" ) );
    executeSql( QString( "CREATE INDEX url_idx%1 ON tags%2( url );" )
                .arg( temporary ? "_temp" : "" ).arg( temporary ? "_temp" : "" ) );

    if ( !temporary )
    {
        executeSql( "CREATE INDEX album_tag ON tags( album );" );
        executeSql( "CREATE INDEX artist_tag ON tags( artist );" );
        executeSql( "CREATE INDEX genre_tag ON tags( genre );" );
        executeSql( "CREATE INDEX year_tag ON tags( year );" );
        executeSql( "CREATE INDEX url_tag ON tags( url );" );

        // create directory statistics database
        executeSql( QString( "CREATE TABLE directories ("
                            "dir VARCHAR(100) UNIQUE,"
                            "changedate INTEGER );" ) );
    }
}


void
CollectionDB::dropTables( bool temporary )
{
    qDebug() << __FUNCTION__;

    executeSql( QString( "DROP TABLE tags%1;" ).arg( temporary ? "_temp" : "" ) );
    executeSql( QString( "DROP TABLE album%1;" ).arg( temporary ? "_temp" : "" ) );
    executeSql( QString( "DROP TABLE artist%1;" ).arg( temporary ? "_temp" : "" ) );
    executeSql( QString( "DROP TABLE genre%1;" ).arg( temporary ? "_temp" : "" ) );
    executeSql( QString( "DROP TABLE year%1;" ).arg( temporary ? "_temp" : "" ) );

    if ( !temporary )
    {
        executeSql( QString( "DROP TABLE directories"));
    }

    // force to re-read over all count for random entry
    p->resultCount=0;
}


void
CollectionDB::moveTempTables()
{
    executeSql( "INSERT INTO tags SELECT * FROM tags_temp;" );
    executeSql( "INSERT INTO album SELECT * FROM album_temp;" );
    executeSql( "INSERT INTO artist SELECT * FROM artist_temp;" );
    executeSql( "INSERT INTO genre SELECT * FROM genre_temp;" );
    executeSql( "INSERT INTO year SELECT * FROM year_temp;" );

    // Re-create index to be fast as possible
    executeSql( QString( "REINDEX album_idx;" ));
    executeSql( QString( "REINDEX artist_idx;" ));
    executeSql( QString( "REINDEX genre_idx;" ));
    executeSql( QString( "REINDEX year_idx;" ));
    executeSql( QString( "REINDEX url_idx;" ));
    executeSql( QString( "REINDEX album_tag;" ));
    executeSql( QString( "REINDEX artist_tag;" ));
    executeSql( QString( "REINDEX genre_tag;" ));
    executeSql( QString( "REINDEX year_tag;" ));
    executeSql( QString( "REINDEX url_tag;" ));
}


void
CollectionDB::createStatsTable()
{
    qDebug() << __FUNCTION__;

    // create music statistics database
    executeSql( QString( "CREATE TABLE statistics ("
                      "url VARCHAR(100) UNIQUE,"
                      "createdate INTEGER,"
                      "accessdate INTEGER,"
                      "playcounter INTEGER );" ) );
}


void
CollectionDB::dropStatsTable()
{
    qDebug() << __FUNCTION__;

    executeSql( "DROP TABLE statistics;" );
}


void
CollectionDB::purgeDirCache()
{
    executeSql( "DELETE FROM directories;" );
}


ulong CollectionDB::getValueID( QString name, QString value, bool autocreate, bool useTempTables )
{

    if ( useTempTables )
        name.append( "_temp" );

    QString command = QString( "SELECT id FROM %1 WHERE name LIKE '%2';" )
                      .arg( name )
                      .arg( escapeString( value ) );
    long id = selectSqlNumber( command );

    //check if item exists. if not, should we autocreate it?
    if ( id < 0 && autocreate )
    {
        command = QString( "INSERT INTO %1 ( name ) VALUES ( '%2' );" )
                  .arg( name )
                  .arg( escapeString( value ) );

        executeSql( command );
        return  p->query->lastInsertId().toInt();
    }

    return id;
}

QStringList CollectionDB::getRandomEntry(QString path, QString genre, QString artist) {

    // retrieve Max_Count

    if ( genre != p->lastGenre
        || artist != p->lastArtist
        || path != p->lastPath
        || p->resultCount == 0 ) {
          //new filter > get new count
          p->lastGenre = genre;
          p->lastArtist = artist;
          p->lastPath = path;

          p->resultCount = getCount(path, genre, artist);
     }

    if (p->resultCount >0 ) {
        long randomID = (qrand() % p->resultCount);
        //qebug() << QString::number(randomID);
        QList<QStringList> entries = selectRandomEntry(QString::number(randomID), path, genre, artist );

        if (!entries.isEmpty())
            return entries.at(0);
         else
            return QStringList();
    }
    else {
        qDebug() << __FUNCTION__ << " No Track found matching filter";
        return QStringList();
    }
}

QStringList CollectionDB::getRandomEntry()
{
    double randMax;

    randMax=RAND_MAX;

    if (p->filterString != p->lastFilterString || p->resultCount == 0)
    {
          //new genre > get new count
          p->lastFilterString = p->filterString;
          p->resultCount=getCount();
     }

    long randomID = (rand() / randMax) * p->resultCount;
    //qDebug() << QString::number(randomID);
    QList<QStringList> entries = selectRandomEntry(QString::number(randomID) );

    if (!entries.isEmpty())
        return entries.at(0);
     else
        return QStringList();
}

ulong CollectionDB::getCount()
{
    QString command = "SELECT count(distinct url) "
                    + p->quickFilter;

    return selectSqlNumber( command );
}


uint CollectionDB::getCount(QString path, QString genre, QString artist)
{
  QString command = "SELECT count(distinct tags.url), sum(tags.length)  FROM tags, artist, genre "
                    " WHERE tags.artist = artist.id "
                    " AND tags.artist = artist.id "
                    " AND tags.genre = genre.id "
                    + p->selectionFilterForRandom(path, genre, artist) +
                    ";";

    QStringList result = selectSql(command).at(0);

    p->resultLength = result[1].toLong();

    return result[0].toLong();
}

long CollectionDB::lastLengthSum()
{
  return p->resultLength;
}

uint CollectionDB::lastMaxCount()
{
  return p->resultCount;
}

QList<QStringList> CollectionDB::selectRandomEntry( QString rownum, QString path, QString genre, QString artist)
{
    QString command = "SELECT tags.url, artist.name, tags.title, album.name, year.name, genre.name, tags.track, tags.length "
            + p->quickFilter
            + p->selectionFilterForRandom(path, genre, artist) +
            " LIMIT 1 OFFSET " + rownum +";";

  return selectSql(command);
}

QList<QStringList> CollectionDB::selectYears()
{
    QString command = "SELECT DISTINCT year.name "
         + p->quickFilter +
         "AND year.name <> '' "
         "ORDER BY year.name DESC;";

  return selectSql(command);
}

QList<QStringList> CollectionDB::selectGenres()
{
    QString command = "SELECT DISTINCT genre.name "
         + p->quickFilter +
         "AND genre.name <> '' "
         "ORDER BY genre.name;";

  return selectSql(command);
}

QList<QStringList> CollectionDB::selectArtists(QString year, QString genre)
{
    QString command = "SELECT DISTINCT artist.name "
         + p->quickFilter
          + p->selectionFilter(year,genre) +
            "AND artist.name <> '' "
         "ORDER BY artist.name;";

  return selectSql(command);
}


QList<QStringList> CollectionDB::selectAlbums(QString year, QString genre, QString artist)
{

  QString command = "SELECT DISTINCT album.name "
            + p->quickFilter
          + p->selectionFilter(year,genre,artist) +
          "AND album.name <> '' "
            "ORDER BY album.name;";

  return selectSql(command);
}


QList<QStringList> CollectionDB::selectTracks(QString year, QString genre, QString artist, QString album)
{
  QString command = "SELECT DISTINCT tags.url, artist.name, tags.title, album.name, year.name, genre.name, tags.track, tags.length "
          + p->quickFilter
          + p->selectionFilter(year,genre,artist,album) +
          "ORDER BY artist.name DESC, album.name DESC, tags.track;";

  return selectSql(command);
}


