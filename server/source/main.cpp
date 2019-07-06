/**
 * @brief SCD Topic Server
 *
 *        SCD Topic Sever is a Web Socket Server which allow you to notify events to all client processes
 *        subscribed to the same topic.
 *
 * @author Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com
 *
 * @copyright (c) 2019 (MIT) Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/sc-develop/
 *
 */
#include <QCoreApplication>
#include "scdtopicserver.h"
#include <QSettings>

#define  echo QTextStream(stderr) <<

/**
 * @brief main server main function
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char *argv[])
{
   QCoreApplication a(argc, argv);

   echo "SC-Develop Web Socket Topic Server v1.0\n";
   echo "Copyright (c) 2019 (MIT) Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com\n";
   echo "https://github.com/sc-develop - git.sc.develop@gmail.com\n\n";

   QString appPath = a.applicationDirPath();

   QSettings cfg(appPath + "/config.cfg",QSettings::IniFormat);

   int port = cfg.value("port",22345).toInt();

   cfg.setValue("port",port);

   cfg.sync();

   SCDTopicServer srv(0,port);

   if (srv.start())
   {
      a.exec();
   }

   return 0;
}
