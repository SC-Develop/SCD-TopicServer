/**

  altri metodi da implementare:

   - delete thumbnail (in questa maniera si può richiesedere di nuovo la thumbnail che verrà ricreata)
   - stop stop current download upload command (for multiple transfer stop ttranfer of remaining file)
   - web interface per configurare il server:

     creazione repository (database) : si imposta la cartella radice sul server che conterrà i database
     ci possono essere più cartelle radici su montaggi diffrenti si locali che di rete
     sotto ogni cartella radcie si possono memorizzare i repository (database)

     non si possono eseguire operazioni su cartelle superiori alla radice.

     impostare un comando che richiede le radici abilitate.

     le radici abiitate sono depositate in un file di configurazione

     aggiungere un comando di creazione repository.

     in seguito aggiungere gli tutenti che passoono accedere ai vari repository presenti soso una data radice

     privilagio di radice, o di database sottto data radice

     anche questi privilegi sonos critti in un file.

     solo l'amminsitratotre del server potrà impostare i rivilegi e la configurazione.
*/

#ifndef SCDTOPICSERVERTHREAD_H
#define SCDTOPICSERVERTHREAD_H

#include <QThread>
#include <QTcpSocket>
#include <QByteArray>
#include <QFile>
#include <QWebSocket>

#include "scdtopicserver.h"

/**
 * @brief The SCDTopicServerThread class
 */
class SCDTopicServerThread : public QThread
{
   Q_OBJECT

   public:

     explicit SCDTopicServerThread(SCDTopicServer *parent = 0);

     void run(); // thread execution

     qintptr getSocketDescriptor() {return socketDescriptor;}

     //SCDTopicServer *server();

   private:

     SCDTopicServer *server;

     qintptr socketDescriptor; // descriptor(handle) of current socket
};

/**
 * @brief The WebSocketHandler class
 */
class WebSocketHandler : public QWebSocket
{
   Q_OBJECT

   public:

     explicit WebSocketHandler(SCDTopicServer *server=0, QWebSocket *socket=0);

     QString lastError();

     QMap <QString,QVariant> getHeader();

   signals:

   private slots:

     void onTextMessageReceived(QString message);
     void onDisconnected();
     void onSocketError(QAbstractSocket::SocketError error);

   private:

     enum Command{TMK,TDL,TRC,TDC,TSM};

     SCDTopicServer *server;

     QWebSocket *socket;  // current connection socket

     int command;          // current reading status

     QString fileName;
     QString lastErrorMsg;

     QMap <QString,QVariant> header; // current header entries readed

     int maxHeaderSize;

     int checkHeaderField(const QString &headerItem, const QString &fieldName);
     int readHeader(QString message);
};

#endif // SCDTOPICSERVERTHREAD_H
