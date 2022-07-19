#include "bookoptionsdialog.h"
#include "ui_bookoptionsdialog.h"

#include "functions.h"

bookOptionsDialog::bookOptionsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::bookOptionsDialog)
{
    ui->setupUi(this);
    ui->pinBtn->setProperty("type", "borderless");
    if(global::localLibrary::bookOptionsDialog::deleteOption == false) {
        global::localLibrary::bookOptionsDialog::deleteOption = true;
        ui->deleteBtn->hide();
        ui->line_2->hide();
        ui->deleteBtn->deleteLater();
        ui->line_2->deleteLater();
        this->adjustSize();
    }
    else {
        ui->deleteBtn->setProperty("type", "borderless");
    }
    ui->infoBtn->setProperty("type", "borderless");

    global::localLibrary::bookOptionsDialog::bookPinAction = false;

    bookPath = getBookMetadata(global::localLibrary::bookOptionsDialog::bookID)["BookPath"].toString();
    if(isBookPinned(global::localLibrary::bookOptionsDialog::bookID)) {
        bookPinned = true;
        ui->pinBtn->setText("Unpin");
    }
    else {
        bookPinned = false;
    }
}

bookOptionsDialog::~bookOptionsDialog()
{
    delete ui;
}

void bookOptionsDialog::on_pinBtn_clicked()
{
    if(bookPinned == false) {
        pinBook(global::localLibrary::bookOptionsDialog::bookID);
    }
    else {
        unpinBook(global::localLibrary::bookOptionsDialog::bookID);
    }
}

void bookOptionsDialog::on_deleteBtn_clicked()
{
    log("Deleting book '" + bookPath + "'", className);
    global::toast::delay = 3000;
    if(QFile::remove(bookPath)) {
        emit showToast("Book deleted successfully");
        global::localLibrary::bookOptionsDialog::bookDeleted = true;
        QFile::remove(global::localLibrary::databasePath);
    }
    else {
        emit showToast("Failed to delete book");
    }
}

void bookOptionsDialog::on_infoBtn_clicked()
{
    emit openLocalBookInfoDialog();
}

void bookOptionsDialog::pinBook(int bookID) {
    QJsonObject pinnedBooksObject;
    if(QFile::exists(global::localLibrary::pinnedBooksDatabasePath)) {
        QString function = __func__; log(function + ": Reading pinned books database", className);
        QFile database(global::localLibrary::pinnedBooksDatabasePath);
        QByteArray data;
        if(database.open(QIODevice::ReadOnly)) {
            data = database.readAll();
            database.close();
        }
        else {
            QString function = __func__; log(function + ": Failed to open pinned books library database file for reading at '" + database.fileName() + "'", className);
        }
        pinnedBooksObject = QJsonDocument::fromJson(qUncompress(QByteArray::fromBase64(data))).object();
        bool bookIsAlreadyPinned = false;
        bool foundSpaceForBook = false;
        for(int i = 1; i <= global::homePageWidget::pinnedBooksNumber; i++) {
            QString pinnedBookPath = getBookMetadata(bookID)["BookPath"].toString();
            if(pinnedBooksObject["Book" + QString::number(i)].toObject().value("BookPath").toString() == pinnedBookPath) {
                bookIsAlreadyPinned = true;
            }
            else {
                if(pinnedBooksObject["Book" + QString::number(i)].toObject().value("BookPath").toString().isEmpty() && foundSpaceForBook == false) {
                    foundSpaceForBook = true;
                    QJsonObject jsonObject;
                    jsonObject.insert("BookPath", pinnedBookPath);
                    pinnedBooksObject["Book" + QString::number(i)] = jsonObject;

                    QString function = __func__; log(function + ": Pinned book with ID " + QString::number(global::localLibrary::bookOptionsDialog::bookID), className);
                    global::toast::delay = 3000;
                    emit showToast("Book pinned successfully");
                    global::localLibrary::bookOptionsDialog::bookPinAction = true;
                }
            }
        }
        if(foundSpaceForBook == false && bookIsAlreadyPinned == false) {
            global::localLibrary::bookOptionsDialog::bookPinAction = false;
            emit showToast("No space left for pinning book");
        }
        else if(bookIsAlreadyPinned == true) {
            global::localLibrary::bookOptionsDialog::bookPinAction = false;
            emit showToast("Book is already pinned");
        }
    }
    else {
        QJsonObject mainJsonObject;
        QJsonObject firstJsonObject;
        firstJsonObject.insert("BookPath", QJsonValue(getBookMetadata(bookID)["BookPath"].toString()));
        mainJsonObject["Book1"] = firstJsonObject;

        for(int i = 2; i <= global::homePageWidget::pinnedBooksNumber; i++) {
            QJsonObject jsonObject;
            jsonObject.insert("BookPath", QJsonValue(""));
            mainJsonObject["Book" + QString::number(i)] = jsonObject;
        }
        pinnedBooksObject = mainJsonObject;
    }
    // Writing database to file
    QFile::remove(global::localLibrary::pinnedBooksDatabasePath);
    writeFile(global::localLibrary::pinnedBooksDatabasePath, qCompress(QJsonDocument(pinnedBooksObject).toJson()).toBase64());
}

void bookOptionsDialog::unpinBook(int bookID) {
    QJsonObject pinnedBooksObject;
    QString function = __func__; log(function + ": Reading pinned books database", className);
    QFile database(global::localLibrary::pinnedBooksDatabasePath);
    QByteArray data;
    if(database.open(QIODevice::ReadOnly)) {
        data = database.readAll();
        database.close();
    }
    else {
        QString function = __func__; log(function + ": Failed to open pinned books library database file for reading at '" + database.fileName() + "'", className);
    }
    pinnedBooksObject = QJsonDocument::fromJson(qUncompress(QByteArray::fromBase64(data))).object();

    QJsonObject mainJsonObject;

    // Removing pinned book associated to requested ID from database
    int bookToUnpin;
    for(int i = 1; i <= global::homePageWidget::pinnedBooksNumber; i++) {
        if(pinnedBooksObject["Book" + QString::number(i)].toObject().value("BookPath").toString() == getBookMetadata(bookID)["BookPath"].toString()) {
            bookToUnpin = i;
        }
    }

    // Recreating pinned books database without previously pinned book
    QString pinnedBookPath;
    int recreationIndex = 1;
    for(int i = 1; i <= global::homePageWidget::pinnedBooksNumber; i++) {
        pinnedBookPath = pinnedBooksObject["Book" + QString::number(i)].toObject().value("BookPath").toString();
        if(i != bookToUnpin) {
            QJsonObject jsonObject;
            jsonObject.insert("BookPath", QJsonValue(pinnedBookPath));
            mainJsonObject["Book" + QString::number(recreationIndex)] = jsonObject;
            recreationIndex++;
        }
    }
    log(function + ": Unpinned book with ID " + bookID, className);
    emit showToast("Book unpinned successfully");
    global::localLibrary::bookOptionsDialog::bookPinAction = true;

    // Writing database to file
    pinnedBooksObject = mainJsonObject;
    QFile::remove(global::localLibrary::pinnedBooksDatabasePath);
    writeFile(global::localLibrary::pinnedBooksDatabasePath, qCompress(QJsonDocument(pinnedBooksObject).toJson()).toBase64());
}

bool bookOptionsDialog::isBookPinned(int bookID) {
    QJsonObject pinnedBooksObject;
    if(QFile::exists(global::localLibrary::pinnedBooksDatabasePath)) {
        QString function = __func__; log(function + ": Reading pinned books database", className);
        QFile database(global::localLibrary::pinnedBooksDatabasePath);
        QByteArray data;
        if(database.open(QIODevice::ReadOnly)) {
            data = database.readAll();
            database.close();
        }
        else {
            QString function = __func__; log(function + ": Failed to open pinned books library database file for reading at '" + database.fileName() + "'", className);
        }
        pinnedBooksObject = QJsonDocument::fromJson(qUncompress(QByteArray::fromBase64(data))).object();
    }
    QString pinnedBookPath = getBookMetadata(bookID)["BookPath"].toString();
    for(int i = 1; i <= global::homePageWidget::pinnedBooksNumber; i++) {
        if(pinnedBooksObject["Book" + QString::number(i)].toObject().value("BookPath").toString() == pinnedBookPath) {
            bookPinned = true;
            break;
        }
        else {
            bookPinned = false;
        }
    }
    return bookPinned;
}
