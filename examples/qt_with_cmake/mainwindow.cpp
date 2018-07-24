#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->label->setText("Have a nice day!");
    this->setWindowTitle("Simple Calendar App");

//    ui->se
}

MainWindow::~MainWindow()
{
    delete ui;
}
