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

#ifndef PROGRESSBAR_H
#define PROGRESSBAR_H

#include <QToolButton>
#include <QProgressBar>
#include <QWidget>

class ProgressBar:public QWidget
{
  Q_OBJECT
  public:
    ProgressBar(QWidget *parent = 0, const char *name = 0);
    ~ProgressBar();
    QToolButton*  clearButton;
    QProgressBar* bar;

    int value();

    public slots:
    void setValue(int value);

signals:
    void stopped();

private:
    void resizeEvent(QResizeEvent *);
      
};
      
#endif
