#include <QApplication>
#include <QtGui>
#include <QtGui/QSystemTrayIcon>
#include <QtGui/QDialog>
#include "mainwindowimpl.h"

#include <QDesktopWidget>

void center(QWidget &widget)
{
  int x, y;
  int screenWidth;
  int screenHeight;

  int WIDTH = 680;
  int HEIGHT = 425;

  QDesktopWidget *desktop = QApplication::desktop();

  screenWidth = desktop->width();
  screenHeight = desktop->height();

  x = (screenWidth - WIDTH) / 2;
  y = (screenHeight - HEIGHT) / 2;

  widget.setGeometry(x, y, WIDTH, HEIGHT);
  widget.setFixedSize(WIDTH, HEIGHT);
}


int main(int argc, char ** argv)
{
    //Q_INIT_RESOURCE(emf_Interface);
    QApplication app( argc, argv );
    //if (!QSystemTrayIcon::isSystemTrayAvailable())
    //{
	//QMessageBox::critical(0, QObject::tr("Systray"),QObject::tr("I couldn't detect any system tray""on this system."));
	//return 1;
    //}
    QApplication::setQuitOnLastWindowClosed(false);

    MainWindowImpl win;
    win.hide(); 
    //center(win);
    app.connect( &app, SIGNAL( lastWindowClosed() ), &app, SLOT( quit() ) );
    return app.exec();
}
