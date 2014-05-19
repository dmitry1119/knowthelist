/*
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

#include <QtDebug>
#include <QtGui>
#include <QtSql>

#include "settingsdialog.h"
#include "ui_settingsdialog.h"

struct SettingsDialog::Private
{
	Ui::SettingsDialog ui;
        QWidget *parent;

};

SettingsDialog::SettingsDialog(QWidget * parent)
	: QDialog(parent)
{
	p = new Private;
	p->ui.setupUi(this);
        p->parent=parent;

	// set icons in the settings list

	QTableWidgetItem * item;
	p->ui.settingsGroupsTable->setIconSize(QSize(32,32));

	item = p->ui.settingsGroupsTable->item(0, 0);
	item->setIcon(QIcon(":preferences-other.png"));
	item->setTextAlignment(Qt::AlignCenter);
        item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);

	item = p->ui.settingsGroupsTable->item(1, 0);
	item->setIcon(QIcon(":tools-media-optical-format.png"));
	item->setTextAlignment(Qt::AlignCenter);
        item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);

	item = p->ui.settingsGroupsTable->item(2, 0);
	item->setIcon(QIcon(":preferences-system-network.png"));
	item->setTextAlignment(Qt::AlignCenter);
        item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);

	item = p->ui.settingsGroupsTable->item(3, 0);
	item->setIcon(QIcon(":server-database.png"));
	item->setTextAlignment(Qt::AlignCenter);
        item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);

	item = p->ui.settingsGroupsTable->item(4, 0);
	item->setIcon(QIcon(":preferences-desktop-keyboard.png"));
	item->setTextAlignment(Qt::AlignCenter);
        item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);

    item = p->ui.settingsGroupsTable->item(5, 0);
    item->setIcon(QIcon(":server-database.png"));
    item->setTextAlignment(Qt::AlignCenter);
        item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);

	int w = p->ui.settingsGroupsTable->width();
	p->ui.settingsGroupsTable->setColumnWidth(0, w);

	// select first item
	p->ui.settingsGroupsTable->setCurrentCell(0, 0);

	// select first page
	p->ui.pages->setCurrentIndex(0);


    connect(p->ui.settingsGroupsTable, SIGNAL(itemSelectionChanged()),
            this, SLOT(tableSelectionChanged()));

    connect(p->ui.faderEndSlider, SIGNAL(sliderMoved(int)),
                    this, SLOT(on_faderEndSlider_sliderMoved(int)));
    connect(p->ui.faderTimeSlider, SIGNAL(sliderMoved(int)),
                    this, SLOT(on_faderTimeSlider_sliderMoved(int)));
}

SettingsDialog::~SettingsDialog()
{
	delete p;
}

int SettingsDialog::exec()
{
	// load settings
	if (!loadSettings()) {
		return QDialog::Rejected;
	}

	return QDialog::exec();
}

void SettingsDialog::accept()
{
    QSettings settings;
    settings.setValue("MonitorOutputDevice",p->ui.monitorOutputDevice->currentText());

    //save fade slider
    settings.setValue("faderTimeSlider",p->ui.faderTimeSlider->value());
    settings.setValue("faderEndSlider",p->ui.faderEndSlider->value());

    //Playlist settings
    settings.setValue("checkAutoRemove",p->ui.checkAutoRemove->isChecked());

    //Silent settings
    settings.setValue("checkAutoCue",p->ui.checkAutoCue->isChecked());
    settings.setValue("checkSkipSilentEnd",p->ui.checkSkipSilentEnd->isChecked());

    //AutoDJ
    settings.setValue("minTracks",p->ui.minTracks->value());
    settings.setValue("countDJ",p->ui.countDJ->value());
    settings.beginGroup("AutoDJ");
    settings.setValue("0/FilterCount","4");
    settings.setValue("1/FilterCount","5");
    settings.setValue("2/FilterCount","3");
    settings.setValue("0/Name","DJ Iam");
    settings.setValue("1/Name","DJ Bear");
    settings.setValue("2/Name","DJ All");
    settings.endGroup();

    //File Browser
    settings.setValue("editBrowerRoot",p->ui.txtBrowserRoot->text());

    QDialog::accept();
}

bool SettingsDialog::loadSettings()
{
    QSettings settings;
    p->ui.monitorOutputDevice->addItems(settings.value("MonitorOutputDevices").toStringList());

    int index = p->ui.monitorOutputDevice->findText(settings.value("MonitorOutputDevice").toString());
    p->ui.monitorOutputDevice->setCurrentIndex(index);

    //fade slider
    p->ui.faderTimeSlider->setValue(settings.value("faderTimeSlider","12").toInt());
    p->ui.faderTimeLabel->setText(settings.value("faderTimeSlider","12").toString() + "s");
    p->ui.faderEndSlider->setValue(settings.value("faderEndSlider","12").toInt());
    p->ui.faderEndLabel->setText(settings.value("faderEndSlider","12").toString() + "s");

    //Playlist setting
    p->ui.checkAutoRemove->setChecked(settings.value("checkAutoRemove",true).toBool());

    //Silent setting
    p->ui.checkSkipSilentEnd->setChecked(settings.value("checkSkipSilentEnd",true).toBool());
    p->ui.checkAutoCue->setChecked(settings.value("checkAutoCue",true).toBool());

    //AutoDJ
    p->ui.minTracks->setValue(settings.value("minTracks","6").toInt());
    p->ui.countDJ->setValue(settings.value("countDJ","1").toInt());


    //File Browser
    p->ui.txtBrowserRoot->setText(settings.value("editBrowerRoot","").toString());

    return true;
}


void SettingsDialog::on_pushButton_clicked()
{
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::DirectoryOnly);
    if (dialog.exec())
         p->ui.txtBrowserRoot->setText(dialog.selectedFiles().first());
}

void SettingsDialog::on_faderEndSlider_sliderMoved(int position)
{
    p->ui.faderEndLabel->setText(QString::number(position)+"s");
}

void SettingsDialog::on_faderTimeSlider_sliderMoved(int position)
{
    p->ui.faderTimeLabel->setText(QString::number(position)+"s");
}

void SettingsDialog::tableSelectionChanged()
{
        QTableWidgetItem * item = p->ui.settingsGroupsTable->selectedItems().first();
        p->ui.pages->setCurrentIndex(item->row());
}
