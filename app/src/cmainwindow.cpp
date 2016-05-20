#include "cmainwindow.h"
#include "ui_cmainwindow.h"

#include "settings/csettings.h"
#include "uisettings.h"

#include <QFileDialog>
#include <QFontDialog>
#include <QPropertyAnimation>
#include <QSlider>
#include <QSpinBox>
#include <QToolBar>

CMainWindow::CMainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::CMainWindow),
	_reader(this)
{
	ui->setupUi(this);

	initToolBars();
	initActions();

	_textFadeEffect.setOpacity(1.0f);
	_textFadeOutAnimation = new QPropertyAnimation(&_textFadeEffect, "opacity");
	_textFadeOutAnimation->setDuration(100);
	_textFadeOutAnimation->setStartValue(1.0f);
	_textFadeOutAnimation->setEndValue(0.0f);

	_textFadeInAnimation = new QPropertyAnimation(&_textFadeEffect, "opacity");
	_textFadeInAnimation->setDuration(100);
	_textFadeInAnimation->setStartValue(0.0f);
	_textFadeInAnimation->setEndValue(1.0f);

	connect(_textFadeOutAnimation, &QPropertyAnimation::finished, [this](){
		_textFadeInAnimation->start();
	});

	ui->_text->setGraphicsEffect(&_textFadeEffect);
}

CMainWindow::~CMainWindow()
{
	delete ui;
}

void CMainWindow::initToolBars()
{
// Reading settings toolbar
	// Font size
	_textSizeSlider = new QSlider(Qt::Horizontal);
	_textSizeSlider->setMinimum(20);
	_textSizeSlider->setMaximum(300);
	_textSizeSlider->setValue(CSettings().value(UI_FONT_SIZE_SETTING, UI_FONT_SIZE_DEFAULT).toInt());

	connect(_textSizeSlider, &QSlider::valueChanged, [this](int size){
		QFont font = ui->_text->font();
		font.setPointSize(size);
		ui->_text->setFont(font);

		CSettings().setValue(UI_FONT_SIZE_SETTING, size);
	});
	_textSizeSlider->valueChanged(_textSizeSlider->value());

	_readingSettingsToolbar = addToolBar(tr("Reading settings"));
	_readingSettingsToolbar->addWidget(new QLabel(tr("Text size") + "  "));
	_readingSettingsToolbar->addWidget(_textSizeSlider);

	// Reading speed
	_readingSettingsToolbar->addSeparator();
	_readingSettingsToolbar->addWidget(new QLabel(tr("Reading speed") + "  "));

	_readingSpeedSlider = new QSlider(Qt::Horizontal);
	_readingSpeedSpinBox = new QSpinBox();

	_readingSpeedSlider->setMinimum(100);
	_readingSpeedSpinBox->setMinimum(100);
	_readingSpeedSlider->setMaximum(800);
	_readingSpeedSpinBox->setMaximum(800);

	_readingSpeedSlider->setTickInterval(50);
	_readingSpeedSlider->setTickPosition(QSlider::TicksBothSides);

	_readingSpeedSpinBox->setSuffix(" WPM");
	_readingSpeedSpinBox->setAccelerated(true);

	connect(_readingSpeedSlider, &QSlider::valueChanged, _readingSpeedSpinBox, &QSpinBox::setValue);
	connect(_readingSpeedSpinBox, (void (QSpinBox::*)(int))&QSpinBox::valueChanged, _readingSpeedSlider, &QSlider::setValue);

	connect(_readingSpeedSlider, &QSlider::valueChanged, [this](int WPM){
		_reader.setReadingSpeed(WPM);
	});

	_readingSpeedSlider->setValue(_reader.readingSpeed());

	_readingSettingsToolbar->addWidget(_readingSpeedSlider);
	_readingSettingsToolbar->addWidget(_readingSpeedSpinBox);
}

void CMainWindow::initActions()
{
	connect(ui->action_Font, &QAction::triggered, [this](){
		QFontDialog fontDialog(ui->_text->font(), this);
		connect(&fontDialog, &QFontDialog::fontSelected, [this](const QFont &font){
			ui->_text->setFont(font);
			_textSizeSlider->setValue(font.pointSize());
		});
		fontDialog.exec();
	});

	ui->actionOpen->setIcon(QApplication::style()->standardIcon(QStyle::SP_DirOpenIcon));
	connect(ui->actionOpen, &QAction::triggered, [this](){
		const QString filePath = QFileDialog::getOpenFileName(this, tr("Pick a text file to open"), CSettings().value(UI_OPEN_FILE_LAST_USED_DIR_SETTING).toString());
		if (!filePath.isEmpty())
		{
			CSettings().setValue(UI_OPEN_FILE_LAST_USED_DIR_SETTING, filePath);
			_reader.loadFromFile(filePath);
		}
	});

	ui->action_Read->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPlay));
	connect(ui->action_Read, &QAction::triggered, [this](){
		if (_reader.state() == CReader::Paused)
			_reader.resumeReading();
		else
			_reader.pauseReading();
	});

	ui->action_Pause->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPause));
	connect(ui->action_Pause, &QAction::triggered, [this](){
		_reader.pauseReading();
	});

	ui->actionStop->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaSkipBackward));
	connect(ui->actionStop, &QAction::triggered, [this](){
		_reader.resetAndStop();
	});

	connect(ui->action_Exit, &QAction::triggered, qApp, &QApplication::exit);
}

void CMainWindow::displayText(const TextFragment& text)
{
	connect(_textFadeOutAnimation, &QPropertyAnimation::finished, _textFadeOutAnimation, [this, text]() {
		ui->_text->setText(text._text);
	}, (Qt::ConnectionType)(Qt::QueuedConnection | Qt::UniqueConnection));

	_textFadeOutAnimation->start();

}

void CMainWindow::stateChanged(const CReader::State newState)
{
	if (newState == CReader::Reading)
	{
		ui->action_Pause->setEnabled(true);
	}
	else if (newState == CReader::Paused)
	{
		ui->action_Pause->setEnabled(false);
	}
}
