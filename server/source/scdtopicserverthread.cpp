/**
 * @class SCDTopicServerThread - https://github.com/sc-develop
 *
 * @brief SCD Image Server Thread management
 *
 *        This is a this part of SCD Image Server
 *
 * @author Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com
 *
 * @copyright (c) 2019 Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/sc-develop/
*/

#include <QHostAddress>
#include <QTcpSocket>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QImageReader>

#include "scdtopicserverthread.h"

/**
 * @brief SCDTopicServerThread::SCDTopicServerThread constructor
 * @param Id
 * @param parent
 */
SCDTopicServerThread::SCDTopicServerThread(SCDTopicServer *parent): QThread(parent), server(parent)
{

}

/**
 * @brief SCDTopicServerThread::run => thread main function
 */
void SCDTopicServerThread::run()
{
   qDebug() << "Starting new connection thread...";

   QWebSocket *socket = server->nextPendingConnection();

   if (socket)                // set a socket descriptor of new allocated socket object
   {
      WebSocketHandler sh(server,socket);

      //qDebug() << "Accepted connection from host: " << " Address: " << socket->peerAddress().toString() << ":" << socket->peerPort();

      exec(); // starts event loop and waits until event loop exits
   }
   else
   {
      qDebug() << "Error: Unable to accept new conenction!";
   }

   delete socket; // explict socket deleting

   qDebug() << "Connection thread end: ";
}

/**
 * @class WebSocketHandler - https://github.com/sc-develop
 *
 * @brief SCD Signals Handler for server threads, this class manage the signals emitted by
 *        the server thread TCP Socket connection. This class live into thread space, and process
 *        the signal of thread event loop.
 *
 *        This is a this part of SCD Image Server
 *
 * @author Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com
 *
 * @copyright (c) 2019 Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/sc-develop/
*/

/**
 * @brief WebSocketHandler::WebSocketHandler
 * @param socket
 * @param mc
 */
WebSocketHandler::WebSocketHandler(SCDTopicServer *server, QWebSocket *socket) : server(server), socket(socket)
{
   connect(socket, SIGNAL(textMessageReceived(QString)),this,SLOT(onTextMessageReceived(QString)));
   connect(socket, SIGNAL(disconnected()),this,SLOT(onDisconnected()));
   connect(socket, SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(onSocketError(QAbstractSocket::SocketError)));

   maxHeaderSize = 1024;
}

/**
 * @brief WebSocketHandler::lastError
 * @return
 */
QString WebSocketHandler::lastError()
{
   return lastErrorMsg;
}

/**
 * @brief WebSocketHandler::getHeader return packet header
 * @return
 */
QMap<QString, QVariant> WebSocketHandler::getHeader()
{
   return header;
}

/**
 * @brief WebSocketHandler::readyRead read data if available
 */
void WebSocketHandler::onTextMessageReceived(QString message)
{
   if (readHeader(message))
   {
      switch (command)
      {
         case TMK: // make a new topic

          // emit newTopic(header["TMK"].toString(), socket->peerAddress().toIPv4Address());

          server->addTopic(header["TMK"].toString());

         break;

         case TDL: // delete a topic

            //emit deleteTopic(header["TMK"].toString(), socket->peerAddress().toIPv4Address());

           server->removeTopic(header["TDL"].toString());

         break;

         case TRC: // register a client to topic

           // emit registerClient(header["TRC"].toString(), socket->peerAddress().toIPv4Address());

           server->addClientToTopic(header["TRC"].toString(),header["address"].toString());

         break;

         case TDC: // remove a client form topic

           // emit removeClient(header["TDC"].toString(), socket->peerAddress().toIPv4Address());

           server->removeClientFromTopic(header["TDC"].toString(),header["address"].toString());

         break;

         case TSM: // send a message to topic

         break;
      }

      return;
   }

   // on error ---------------------------------------

   qDebug() << lastErrorMsg;

   socket->sendTextMessage(lastErrorMsg.toLatin1()+"\n");
   socket->close();
}

/**
 * @brief WebSocketHandler::disconnected exit from event thread loop (thread end)
 */
void WebSocketHandler::onDisconnected()
{
   qDebug() << "Client disconnected: " << socket->objectName();

   thread()->quit(); //exit(0); => exit from thread event loop
}

/**
 * @brief WebSocketHandler::onSocketError
 */
void WebSocketHandler::onSocketError(QAbstractSocket::SocketError error)
{
   qDebug() << error << socket->errorString();

   socket->abort();
}

/**
 * @brief WebSocketHandler::checkField split header intem anf get field name and valu,
 *                                   if field name match fieldName param add it to header map and return 1
 *                                   else return 0;
 * @param headerItem header item in format <field name>:<value>
 * @param fieldName
 * @return 1 on success, 0 un failure
 */
int WebSocketHandler::checkHeaderField(const QString &headerItem, const QString &fieldName)
{
   QStringList field = headerItem.split(":"); // split into field name and value pair

   if (field.size() != 2)
   {
      lastErrorMsg = "Bad header item: " + headerItem;
      return 0;
   }

   QString name = field.at(0).trimmed();

   if (name==fieldName)
   {
      header.insert(name,field.at(1).trimmed());
      return 1;
   }

   lastErrorMsg = "Bad header field name: " + headerItem;

   return 0;
}

/**
 * @brief WebSocketHandler::readHeader read headers lines and fill header map.
 *        Call this method only when statu=WAITFORHEADER.
 *        when header read is complete the stustus change to WAITFORDATA.
 *        on error return 0, otherwise return 1.
 *        Do not use the sequence '|' (sharp) char into complete file path
 *
 *        SCDTMH (one line fast header struct)
 *
 *          TMK command => SCDTMH:1.0\tTMK:<topic name>\n                  // make a new topic
 *          TDL command => SCDTMH:1.0\tTDL:<topic name>\n                  // delete a topic
 *          TRC command => SCDTMH:1.0\tTRC:<topic name>\t<address>\n       // register a client to topic
 *          TDC command => SCDTMH:1.0\tTDC:<topic name>\t<address>\n       // delete a client from topic
 *          TSM command => SCDTMH:1.0\tTSM:<topic name>\t<size>\n<message> // send a message to topic
 *
 * @return 1 on success, 0 on failure
 *
 */
int WebSocketHandler::readHeader(QString message)
{
   QTextStream ts(&message);

   QString head = ts.readLine(maxHeaderSize);

   if (head.isEmpty()==0)
   {
      lastErrorMsg = "Null header line";
      return 0;
   }

   QStringList fields = head.split("\t",QString::SkipEmptyParts);

   if (fields.size()<2) // header must have at least two elements
   {
      lastErrorMsg = "Invalid header: " + head;
      return 0;
   }

   if (checkHeaderField(fields.at(0),"SCDTMH")) // first item must be header type declaration
   {
      if (checkHeaderField(fields.at(1),"TSM")) // TOPIC SEND MESSAGE check action to perform (second header item)
      {
         command = TSM;

         if (fields.size()==3)
         {
             QString address = fields.at(2).trimmed();

             header.insert("address", address);

             return 1;
         }

         lastErrorMsg = "Invalid header: " + head;

         return 0;
      }
      else
      if (checkHeaderField(fields.at(1),"TRC")) // TOPIC REGISTER CLIENT check action to perform (second header item)
      {
         command = TRC;

         if (fields.size()==3)
         {
            QString address = fields.at(2).trimmed();

            header.insert("address",address);

            return 1;
         }

         lastErrorMsg = "Invalid header: " + head;

         return 0;
      }
      if (checkHeaderField(fields.at(1),"TDC")) // TOPIC REGISTER CLIENT check action to perform (second header item)
      {
         command = TDC;

         if (fields.size()==3)
         {
            QString address = fields.at(2).trimmed();

            header.insert("address",address);

            return 1;
         }

         lastErrorMsg = "Invalid header: " + head;

         return 0;
      }
      else
      if (checkHeaderField(fields.at(1),"TMK")) // TOPIC MAKE
      {
         command = TMK;
         return 1;
      }
      else
      if (checkHeaderField(fields.at(1),"TDL")) // TOPIC DELETE
      {
         command = TDL;
         return 1;
      }
   }

   return 0;  // failure
}


/**
 * @brief WebSocketHandler::fileReceivingPrepare
 * @return
 */
/*int WebSocketHandler::fileReceivingPrepare(QString fileName)
{
   QDir dir = QFileInfo(fileName).absoluteDir();

   // Create destionation file path if not exists -----------------------------

   if (!dir.mkpath(dir.absolutePath()))
   {
      lastErrorMsg = "Create dir failure: " + dir.absolutePath();
      return 0;
   }

   // remove file if already existing -----------------------------------------

   if (QFile::exists(fileName))
   {
      if (!QFile::remove(fileName))
      {
         lastErrorMsg = "Create dir failure: " + dir.absolutePath();
         return 0;
      }
   }

   // open destionation file for (create/append)--------------------------------

   QString tmpFile = dir.absolutePath() + "/" + QFileInfo(fileName).completeBaseName() + ".tmp";

   f.setFileName(tmpFile);

   if (f.open(QIODevice::WriteOnly))
   {
      readedBytes = 0;
      return 1;
   }

   lastErrorMsg = "open file error: " + f.fileName() + " => " + f.errorString();
   return 0;
}*/

/**
 * @brief WebSocketHandler::readData
 */
/*int WebSocketHandler::readData()
{
   QByteArray buff = socket->readAll(); // read all available data from socket connection

   readedBytes += buff.size();

   if (f.write(buff)!=-1)    // file writing success
   {
      if (readedBytes>=fileSize) // if file is entirely readed close file
      {
         f.close();

         if (f.rename(fileName))
         {
            return 2; // file entirely received
         }
         else
         {
            f.remove(); // delete file

            lastErrorMsg = "Rename file error: " + f.fileName() + " => " + f.errorString();

            return 0;
         }
      }

      return 1; // success buffer received
   }

   // on write error ----------------------------------------------------------

   f.close();  // close file
   f.remove(); // delete file

   lastErrorMsg = "write file error: " + f.fileName() + " => " + f.errorString();

   return 0;
}*/

/**
 * @brief WebSocketHandler::sendFile
 * @return
 */
/*int WebSocketHandler::sendFile(QString fileName)
{
   if (!QFile::exists(fileName))
   {
      lastErrorMsg = "File not exists: " + fileName;
      return 0; // system file error
   }

   // Get file size ----------------------------------

   QFile f(fileName);

   int size = f.size();

   QByteArray buff = QByteArray::number(size);

   buff.append("\n");

   // write header ------------------------------------

   if (socket->write(buff.constData(),buff.size())==-1)
   {
      lastErrorMsg = "Write error";
      return -1; // socket error
   }

   // send file to client -----------------------------

   if (!f.open(QIODevice::ReadOnly))
   {
      lastErrorMsg = "Open file error: " + fileName;
      return 0; // system file error
   }

   buff = f.readAll();

   if (buff.size()==0)
   {
      f.close();
      lastErrorMsg = "Read file error: " + fileName;
      return 0; // system file error
   }

   if (socket->write(buff.constData(),buff.size())==-1)
   {
      f.close();
      lastErrorMsg = "Socket write error";
      return -1; // socket error
   }

   f.close();

   return 1;
}*/

/**
 * @brief WebSocketHandler::delFile
 * @return
 */
/*int WebSocketHandler::delFile(QString fileName)
{
   if (!QFile::remove(fileName))
   {
      lastErrorMsg = "Delete file error: " + fileName + " => " + f.errorString();
      return 0; // system file error
   }

   return 1;
}*/

