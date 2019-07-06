/**
 * @brief SCD Topic Client - Gui Client which allow you to connect to SCD Topic Server and create/subscribe topics
 *
 *        SCD Topic Sever is a Web Socket Server which allow you to notify events to all client processes
 *        subscribed to the same topic.
 *
 * @author Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com
 *
 * @copyright (c) 2019 (MIT) Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/sc-develop/
 *
 */
#include "mainwindow.h"
#include <QApplication>

/**
 * @brief main
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
