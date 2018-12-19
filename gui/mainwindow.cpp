#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "../cui/source/parameters.h"

QString execPathStr = "";    //���s���Ă���R���o�[�^GUI�̃p�X
QString Outputpath = "";     //�o�̓t�H���_
QString cnvOutputStr = "";   //�R���o�[�g����
bool convert_exec = false;  //�R���o�[�g����
bool convert_error = false;  //�R���o�[�g�G���[������������
int convet_index = 0;
std::map<QString, QString> map_sortmode;
std::map<int, QString> map_texture_wh;
std::map<int, QString> map_canvasSize;

template<typename T1, typename T2>
T1 MainWindow::getKey(const std::map<T1, T2> & map, const T2 & value) const
{
    auto itr = map.cbegin();
    for(; itr != map.cend(); itr++)
    {
        if(itr->second == value)
        {
            return itr->first;
        }
    }
    return itr->first;
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    //�t�H�[���̕��i�ɃA�N�Z�X����ꍇ��ui�̃����o���o�R����
    ui->setupUi(this);

    //�h���b�O���h���b�v��L���ɂ���
    setAcceptDrops(true);

    cnvProcess = new QProcess(this);
    // �v���Z�X���I���������� finished �V�O�i�����M
    connect(cnvProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(processFinished(int, QProcess::ExitStatus )));
    // �v���Z�X����G���[�o�͂������ēǂݍ��݉\�ɂȂ����� readyReadStandardError �V�O�i�����M
    connect(cnvProcess, SIGNAL(readyReadStandardError()), this, SLOT(processErrOutput()));

    imgProcess = new QProcess(this);

    //�E�B���h�E�̃^�C�g��������
    setWindowTitle("PSDtoSS6 GUI Ver2.0.0");

    //������
    convert_exec = false;
    cnvOutputStr.clear();

    //�E�B���h�E�T�C�Y�Œ�
    setFixedSize( QSize(688,661) );

    ui->comboBox->addItem(map_sortmode["none"] = "Name");
    ui->comboBox->addItem(map_sortmode["rectmax"] = "Area Size");
    ui->comboBox->addItem(map_sortmode["wmax"] = "Width");
    ui->comboBox->addItem(map_sortmode["hmax"] = "Height");
    ui->comboBox->setCurrentIndex(1);

    ui->comboBox_w->addItem(map_texture_wh[0] = "Auto");
    ui->comboBox_w->addItem(map_texture_wh[256] = "256");
    ui->comboBox_w->addItem(map_texture_wh[512] = "512");
    ui->comboBox_w->addItem(map_texture_wh[1024] = "1024");
    ui->comboBox_w->addItem(map_texture_wh[2048] = "2048");
    ui->comboBox_w->addItem(map_texture_wh[4096] = "4096");

    ui->comboBox_h->addItem(map_texture_wh[0]);
    ui->comboBox_h->addItem(map_texture_wh[256]);
    ui->comboBox_h->addItem(map_texture_wh[512]);
    ui->comboBox_h->addItem(map_texture_wh[1024]);
    ui->comboBox_h->addItem(map_texture_wh[2048]);
    ui->comboBox_h->addItem(map_texture_wh[4096]);

    ui->comboBox_canvasSize->addItem(map_canvasSize[0] = "Default");
    ui->comboBox_canvasSize->addItem(map_canvasSize[1] = "PSD Size");

    pushButton_enableset();

    //�X�^�C���V�[�g��ǂݍ���
    QFile file(":/style.css");
    file.open(QFile::ReadOnly);
    QString styleSheet = QString::fromLatin1(file.readAll());
    qApp->setStyleSheet(styleSheet);

    //Documents�̃p�X���擾
    data_path = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    data_path += "/SpriteStudio/PSDtoSS6";
    QDir dir;
    //�ݒ�t�@�C���ۑ��p�f�B���N�g�����쐬
    dir.mkpath(data_path);
}

MainWindow::~MainWindow()
{
    delete_convert_info();
    delete ui;
}

void MainWindow::loadConfig(const QString & fileName)
{
    convert_parameters cp;
    cp.parseConfigJson(fileName.toStdString());

    ui->comboBox_w              ->setCurrentText(map_texture_wh[cp.tex_w]);
    ui->comboBox_h              ->setCurrentText(map_texture_wh[cp.tex_h]);
    ui->lineEdit_padding_shape  ->setText(QString::number(cp.tex_padding_shape));
    ui->lineEdit_cell_padding   ->setText(QString::number(cp.pack_padding));
    ui->comboBox                ->setCurrentIndex(cp.sortmode);
    ui->checkBox_ssae           ->setChecked(cp.is_ssaeoutput);
    ui->checkBox_sspj           ->setChecked(cp.is_sspjoutput);
    ui->lineEdit_pri            ->setText(QString::number(cp.addpri));
    ui->checkBox_addnull        ->setChecked(cp.is_addnull);
    ui->checkBox_overwrite      ->setChecked(cp.is_overwrite);
    ui->checkBox_pivot          ->setChecked(cp.is_oldPivotUse);
    ui->checkBox_root           ->setChecked(cp.is_rootLayerUse);
    ui->checkBox_pivot_add      ->setChecked(cp.is_layerPivotUse);
    ui->lineEdit_padding_border ->setText(QString::number(cp.padding_border));
    ui->comboBox_canvasSize     ->setCurrentText(map_canvasSize[cp.canvasSize]);
    ui->lineEdit_padding_inner  ->setText(QString::number(cp.inner_padding));
    ui->textBrowser_output      ->setText(cp.outputpath.c_str());
}

void MainWindow::saveConfig(const QString & fileName)
{
    convert_parameters cp;

    cp.tex_w                = getKey(map_texture_wh, ui->comboBox_w->currentText());
    cp.tex_h                = getKey(map_texture_wh, ui->comboBox_h->currentText());
    cp.tex_padding_shape    = ui->lineEdit_padding_shape->text().toInt();
    cp.pack_padding         = ui->lineEdit_cell_padding->text().toInt();
    cp.sortmode             = ui->comboBox->currentIndex();
    cp.is_ssaeoutput        = ui->checkBox_ssae->isChecked();
    cp.is_sspjoutput        = ui->checkBox_sspj->isChecked();
    cp.addpri               = ui->lineEdit_pri->text().toInt();
    cp.is_addnull           = ui->checkBox_addnull->isChecked();
    cp.is_overwrite         = ui->checkBox_overwrite->isChecked();
    cp.is_layerPivotUse     = ui->checkBox_pivot->isChecked();
    cp.is_rootLayerUse      = ui->checkBox_root->isChecked();
    cp.is_oldPivotUse       = ui->checkBox_pivot_add->isChecked();
    cp.padding_border       = ui->lineEdit_padding_border->text().toInt();
    cp.canvasSize           = getKey(map_canvasSize, ui->comboBox_canvasSize->currentText());
    cp.inner_padding        = ui->lineEdit_padding_inner->text().toInt();
    cp.outputpath           = ui->textBrowser_output->toPlainText().toStdString();
    cp.outputname           = "";

    cp.saveConfigJson(fileName.toStdString());
}


void MainWindow::setText_to_List(QStringList list)
{
    //���s�t�@�C���̃p�X��ۑ�
    execPathStr = list[0];

    if ( list.length() > 1 )
    {
        int i;
        for ( i = 1; i < list.length(); i++ )
        {
            QString dragFilePath;
            dragFilePath = list[i];
            if ( ( dragFilePath.endsWith(".txt")) || ( dragFilePath.endsWith(".txt")) )
            {
                ui->listWidget->addItem(dragFilePath);
            }
        }
    }
    loadConfig(data_path + "/config.json");
    templistload();

}

void MainWindow::on_pushButton_exit_clicked()
{
    saveConfig(data_path + "/config.json");
    delete_convert_info();
    //�A�v���P�[�V�����̏I��
    exit(0);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    if(e->mimeData()->hasUrls())
    {
        e->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *e)
{
    if(e->mimeData()->hasUrls())
    {
        QList<QUrl> urlList = e->mimeData()->urls();
        for(int i = 0; i < urlList.size(); i++)
        {
            //�h���b�O�����t�@�C�������X�g�ɒǉ�
            //.png�ȊO�͒e��
            QString dragFilePath;
            dragFilePath = urlList[i].toLocalFile();
            if (
                 ( dragFilePath.endsWith(".ss6-psdtoss6-info"))
              || ( dragFilePath.endsWith(".psd"))
               )
            {
                //�������O�����X�g�ɂ���ꍇ�͒e��
                bool addname = true;
                int j = 0;
                for ( j = 0; j < ui->listWidget->count(); j++ )
                {
                    QString fileName = ui->listWidget->item(j)->text();
                    if ( fileName == dragFilePath )
                    {
                        addname = false;
                        break;
                    }

                }
                if ( addname == true )
                {
                    ui->listWidget->addItem(dragFilePath);
                }
            }
        }
        pushButton_enableset();
    }
}

void MainWindow::on_pushButton_listclear_clicked()
{
    //���X�g�N���A
    ui->listWidget->clear();
    pushButton_enableset();
}

void MainWindow::on_pushButton_convert_clicked()
{
    //�R���o�[�g
    if ( ui->listWidget->count() == 0 )
    {
        QMessageBox msgBox(this);
        msgBox.setText(tr("txt �t�@�C����o�^���Ă�������"));
        msgBox.exec();
        return;
    }
    if ( ui->textBrowser_output->toPlainText() == "" )
    {
        QMessageBox msgBox(this);
        msgBox.setText(tr("�o�̓t�H���_��I�����Ă�������"));
        msgBox.exec();
        return;
    }

    if (( ui->listWidget->count() > 0 ) && (convert_exec == false))
    {
        templistsave();            //�R���o�[�g���X�g��ۑ�
        save_convert_info();//�R���o�[�g���t�@�C����ۑ�
        buttonEnable( false );   //�{�^������
        convert_error = false;
        convert_exec = false;  //�R���o�[�g����
        convet_index = 0;
        ui->textBrowser_err->setText(tr(""));           //�G���[
        cnvOutputStr = "";

        convert_exec = true;  //�R���o�[�g����
        int i;
        for ( i = 0; i < ui->listWidget->count(); i++ )
        {
            //�i�s�󋵕\��
            QString st = QString("Exec %1/%2").arg(i+1).arg(ui->listWidget->count());
            ui->textBrowser_status->setText(st);     //�X�e�[�^�X

            //�R���o�[�g�����쐬
            QString fileName = ui->listWidget->item(i)->text();
            //�R���o�[�^�̋N��
            if (fileName.isEmpty())
            {
                //�t�@�C�����Ȃ�
            }
            else
            {
                QString str;
                QString execstr;

        #ifdef Q_OS_WIN32
                // Windows
                QDir dir = QDir(execPathStr);
                dir.cd("..");
                QString str_current_path = dir.path();
                execstr = str_current_path + "/PSDtoSS6.exe";
        #else
                // Mac
                QDir dir = QDir(execPathStr);
                dir.cd("..");
                dir.cd("..");
                dir.cd("..");
                dir.cd("..");
                QString str_current_path = dir.path();
                execstr = str_current_path + "/PSDtoSS6";
        #endif

                qDebug() << "\n===========================================";
                qDebug() << "Running " << execstr;
                qDebug() << (QFile::exists(execstr) ? "File exists: " : "File may not exist:") << execstr;
                if ( QFile::exists(execstr) == false )
                {
                    //�t�@�C���̗L���𒲂ׂ�
                    convert_error = true;
                    cnvOutputStr = cnvOutputStr + "Convertor file exists false\n";
                    ui->textBrowser_err->setText(cnvOutputStr);
                }

                execstr= "\"" + execstr + "\"";

                str = execstr + " \"" + fileName + "\"";
                cnvProcess->start(str); //�p�X�ƈ���
                
                while ( 1 )
                {
                    QThread::sleep(1);  // Wait
                    QCoreApplication::processEvents();
                    if ( cnvProcess->state() != QProcess::Running )
                    {
//                        ui->textBrowser_status->setText(tr("Convert end"));
                        break;
                    }
                }
                QCoreApplication::processEvents();
                if ( convert_error == true )
                {
                    break;
                }
                convet_index++;
            }
        }
        if ( convert_error == false )
        {
            ui->textBrowser_status->setText(tr("Convert Success!"));
        }
        else
        {
            ui->textBrowser_status->setText(tr("Error"));   //�X�e�[�^�X
        }
        buttonEnable( true );   //�{�^���L��
        convert_exec = false;  //�R���o�[�g����
    }
}

void MainWindow::processErrOutput()
{

    // �o�͂�S�Ď擾
    QByteArray output = cnvProcess->readAllStandardError();
    //���[�j���O�̕\���͍s��Ȃ�
    QString str = QString::fromLocal8Bit( output );
//    int i = str.indexOf("warning");
//    if ( i == -1 )  //���[�j���O�ȊO��\��
    {
        cnvOutputStr = cnvOutputStr + str;
        ui->textBrowser_err->setText(cnvOutputStr);
    }
    //�J�[�\�����ŏI�s�ֈړ�
    QScrollBar *sb = ui->textBrowser_err->verticalScrollBar();
    sb->setValue(sb->maximum());

}
void MainWindow::processFinished( int exitCode, QProcess::ExitStatus exitStatus)
{
    if ( exitStatus == QProcess::CrashExit )
    {
//        QMessageBox::warning( this, tr("Error"), tr("Crashed") );
        cnvOutputStr = cnvOutputStr + "Error:" + ui->listWidget->item(convet_index)->text();
        ui->textBrowser_err->setText(cnvOutputStr);
        convert_error = true;
        //�J�[�\�����ŏI�s�ֈړ�
        QScrollBar *sb = ui->textBrowser_err->verticalScrollBar();
        sb->setValue(sb->maximum());
    }
    else if ( exitCode != 0 )
    {
//        QMessageBox::warning( this, tr("Error"), tr("Failed") );
        cnvOutputStr = cnvOutputStr + "Error:" + ui->listWidget->item(convet_index)->text();
        ui->textBrowser_err->setText(cnvOutputStr);
        convert_error = true;
        //�J�[�\�����ŏI�s�ֈړ�
        QScrollBar *sb = ui->textBrowser_err->verticalScrollBar();
        sb->setValue(sb->maximum());
    }
    else
    {
        convert_error = false;
        // ����I�����̏���
//        ui->textBrowser_status->setText(tr("Convert Success!"));
//    QMessageBox::information(this, tr("Ss6Converter"), tr("Convert success"));
    }
}

//�o�̓t�H���_�I���{�^��
void MainWindow::on_pushButton_output_clicked()
{
    QString str;
    str = QFileDialog::getExistingDirectory(this, tr("Output Directory"), Outputpath);

    if ( str != "" )
    {
        Outputpath = str;
        Outputpath += "/";
        ui->textBrowser_output->setText(Outputpath);
    }

}

//���X�g�̓ǂݍ���
void MainWindow::on_pushButton_listload_clicked()
{
    QFileDialog::Options options;
    QString strSelectedFilter;
    QString fileName;
    fileName = QFileDialog::getOpenFileName(this, tr("select list File"), ".", tr("text(*.txt)"), &strSelectedFilter, options);

    if ( fileName != "" )
    {
        //���X�g�N���A
        ui->listWidget->clear();

        //�ǂݍ��񂾃t�@�C�������X�g�ɐݒ�
        QFile file(fileName);

        if (!file.open(QIODevice::ReadOnly))//�Ǎ��݂̂ŃI�[�v���ł������`�F�b�N
        {
            return;
        }

        QTextStream in(&file);
        while ( !in.atEnd() ) {
            QString str = in.readLine();//1�s�Ǎ�
            ui->listWidget->addItem(str);
        }
    }
    pushButton_enableset();

}

//���X�g�̕ۑ�
void MainWindow::on_pushButton_listsave_clicked()
{
    QFileDialog::Options options;
    QString strSelectedFilter;
    QString fileName;
    fileName = QFileDialog::getSaveFileName(this, tr("save list File"), ".", tr("text(*.txt)"), &strSelectedFilter, options);

    if ( fileName != "" )
    {
        //�ǂݍ��񂾃t�@�C�������X�g�ɐݒ�
        QFile file(fileName);

        if (!file.open(QIODevice::WriteOnly))//�Ǎ��݂̂ŃI�[�v���ł������`�F�b�N
        {
            return;
        }

        QTextStream out(&file);
        int i;
        for ( i = 0; i < ui->listWidget->count(); i++ )
        {
            QString str = ui->listWidget->item(i)->text();
            out << str << endl; //������
        }
    }
}

void MainWindow::buttonEnable( bool flg )
{
    ui->pushButton_fileadd->setEnabled(flg);
    ui->pushButton_listsave->setEnabled(flg);
    ui->pushButton_listload->setEnabled(flg);
    ui->pushButton_listclear->setEnabled(flg);
    ui->pushButton_output->setEnabled(flg);
    ui->pushButton_convert->setEnabled(flg);
    ui->pushButton_exit->setEnabled(flg);
    ui->pushButton_settingsave->setEnabled(flg);
    ui->pushButton_open_help->setEnabled(flg);
}
//�t�@�C���ǉ�
void MainWindow::on_pushButton_fileadd_clicked()
{
    QFileDialog::Options options;
    QString strSelectedFilter;
    QString addfileName;
    addfileName = QFileDialog::getOpenFileName(this, tr("select convert File"), ".", tr("data(*.ss6-psdtoss6-info *.psd)"), &strSelectedFilter, options);

    if ( addfileName != "" )
    {
        //�������O�����X�g�ɂ���ꍇ�͒e��
        bool addname = true;
        int j = 0;
        for ( j = 0; j < ui->listWidget->count(); j++ )
        {
            QString fileName = ui->listWidget->item(j)->text();
            if ( fileName == addfileName )
            {
                addname = false;
                break;
            }

        }
        if ( addname == true )
        {
            ui->listWidget->addItem(addfileName);
        }
    }
    pushButton_enableset();


}

void MainWindow::pushButton_enableset()
{
    bool flg = true;
    if ( ui->listWidget->count() == 0 )
    {
        flg = false;
    }
    ui->pushButton_listsave->setEnabled(flg);
    ui->pushButton_listclear->setEnabled(flg);
    ui->pushButton_convert->setEnabled(flg);
}

//�ꎞ�I�ȃR���o�[�g���X�g�쐬
void MainWindow::templistsave(void)
{
    QString fileName = data_path + "/templist";
    QFile file(fileName);

    if (!file.open(QIODevice::WriteOnly))//�����݂݂̂ŃI�[�v���ł������`�F�b�N
    {
        return;
    }

    QTextStream out(&file);
    int i;
    for ( i = 0; i < ui->listWidget->count(); i++ )
    {
        QString fileName = ui->listWidget->item(i)->text();
        out << fileName << endl;
    }
}
//�ꎞ�I�ȃ��X�g�̓ǂݍ���
void MainWindow::templistload()
{
    QString fileName = data_path + "/templist";

    if ( fileName != "" )
    {
        //���X�g�N���A
        ui->listWidget->clear();

        //�ǂݍ��񂾃t�@�C�������X�g�ɐݒ�
        QFile file(fileName);

        if (!file.open(QIODevice::ReadOnly))//�Ǎ��݂̂ŃI�[�v���ł������`�F�b�N
        {
            return;
        }

        QTextStream in(&file);
        while ( !in.atEnd() ) {
            QString str = in.readLine();//1�s�Ǎ�
            ui->listWidget->addItem(str);
        }
    }
    pushButton_enableset();

}

//�R���o�[�g���t�@�C�����쐬
void MainWindow::save_convert_info()
{
    QString fileName(data_path + "/convert_info.json");
    saveConfig(fileName);
}
//�R���o�[�g���t�@�C�����폜
void MainWindow::delete_convert_info()
{
    QFile file(data_path + "/convert_info.json");
    if(file.exists())
    {
        file.remove();
    }
}

//�ݒ�̕ۑ��{�^��
void MainWindow::on_pushButton_settingsave_clicked()
{
    saveConfig(data_path + "/config.json");

    QMessageBox msgBox(this);
    msgBox.setText(tr("���݂̐ݒ��ۑ����܂���"));
    msgBox.exec();
}

void MainWindow::on_pushButton_open_help_clicked()
{
    //�w���v���u���E�U�ŊJ��
    QUrl url = QUrl( "http://www.webtech.co.jp/help/ja/spritestudio/guide/tool/psdtoss6/" );
    QDesktopServices::openUrl( url );
}
