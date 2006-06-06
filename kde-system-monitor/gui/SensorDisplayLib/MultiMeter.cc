/*
    kde-system-monitor, the KDE System Monitor
   
	Copyright (c) 1999, 2000, 2001 Chris Schlaeger <cs@kde.org>
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of version 2 of the GNU General Public
    License as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#include <math.h>
#include <stdlib.h>

#include <qdom.h>
#include <qlcdnumber.h>
#include <QToolTip>

#include <kdebug.h>

#include <ksgrd/SensorManager.h>
#include <ksgrd/StyleEngine.h>

#include "MultiMeter.moc"
#include "MultiMeterSettings.h"

MultiMeter::MultiMeter(QWidget* parent, const QString& title, bool isApplet)
	: KSGRD::SensorDisplay(parent, title, isApplet)
{
	setShowUnit( true );
	lowerLimit = upperLimit = 0;
	lowerLimitActive = upperLimitActive = false;

	normalDigitColor = KSGRD::Style->firstForegroundColor();
	alarmDigitColor = KSGRD::Style->alarmColor();
	lcd = new QLCDNumber(this);
	Q_CHECK_PTR(lcd);
	lcd->setSegmentStyle(QLCDNumber::Filled);
	setDigitColor(KSGRD::Style->backgroundColor());
	lcd->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,
					   QSizePolicy::Expanding, false));

	setBackgroundColor(KSGRD::Style->backgroundColor());
	/* All RMB clicks to the lcd widget will be handled by 
	 * SensorDisplay::eventFilter. */
	lcd->installEventFilter(this);

	setPlotterWidget(lcd);

	setMinimumSize(16, 16);
	setModified(false);
}

bool
MultiMeter::addSensor(const QString& hostName, const QString& sensorName,
					const QString& sensorType, const QString& title)
{
	if (sensorType != "integer" && sensorType != "float")
		return (false);

	registerSensor(new KSGRD::SensorProperties(hostName, sensorName, sensorType, title));

	/* To differentiate between answers from value requests and info
	 * requests we use 100 for info requests. */
	sendRequest(hostName, sensorName + '?', 100);

	lcd->setToolTip( QString("%1:%2").arg(hostName).arg(sensorName));

	setModified(true);
	return (true);
}

void
MultiMeter::answerReceived(int id, const QStringList& answerlist)
{
	/* We received something, so the sensor is probably ok. */
	sensorError(id, false);
	QString answer;
	if(!answerlist.isEmpty()) answer = answerlist[0];

	if (id == 100)
	{
		KSGRD::SensorIntegerInfo info(answer);
		setUnit(KSGRD::SensorMgr->translateUnit(info.unit()));
	}
	else
	{
		double val = answer.toDouble();
		int digits = (int) log10(val) + 1;

		if (isApplet())
			lcd->setNumDigits(qMin(4,digits));
		else
			lcd->setNumDigits(qMin(5,digits));

		lcd->display(val);
		if (lowerLimitActive && val < lowerLimit)
		{
			setDigitColor(alarmDigitColor);	
		}
		else if (upperLimitActive && val > upperLimit)
		{
			setDigitColor(alarmDigitColor);
		}
		else
			setDigitColor(normalDigitColor);
	}
}

void
MultiMeter::resizeEvent(QResizeEvent*)
{
	lcd->setGeometry(0, 0, width() - 1, height() - 1);
}

bool
MultiMeter::restoreSettings(QDomElement& element)
{
	lowerLimitActive = element.attribute("lowerLimitActive").toInt();
	lowerLimit = element.attribute("lowerLimit").toLong();
	upperLimitActive = element.attribute("upperLimitActive").toInt();
	upperLimit = element.attribute("upperLimit").toLong();

	normalDigitColor = restoreColor(element, "normalDigitColor",
						KSGRD::Style->firstForegroundColor());
	alarmDigitColor = restoreColor(element, "alarmDigitColor",
						KSGRD::Style->alarmColor());
	setBackgroundColor(restoreColor(element, "backgroundColor",
						KSGRD::Style->backgroundColor()));

	addSensor(element.attribute("hostName"), element.attribute("sensorName"), (element.attribute("sensorType").isEmpty() ? "integer" : element.attribute("sensorType")), "");

	SensorDisplay::restoreSettings(element);

	setModified(false);

	return (true);
}

bool
MultiMeter::saveSettings(QDomDocument& doc, QDomElement& element, bool save)
{
	element.setAttribute("hostName", sensors().at(0)->hostName());
	element.setAttribute("sensorName", sensors().at(0)->name());
	element.setAttribute("sensorType", sensors().at(0)->type());
	element.setAttribute("showUnit", showUnit());
	element.setAttribute("lowerLimitActive", (int) lowerLimitActive);
	element.setAttribute("lowerLimit", (int) lowerLimit);
	element.setAttribute("upperLimitActive", (int) upperLimitActive);
	element.setAttribute("upperLimit", (int) upperLimit);

	saveColor(element, "normalDigitColor", normalDigitColor);
	saveColor(element, "alarmDigitColor", alarmDigitColor);
	saveColor(element, "backgroundColor", lcd->backgroundColor());

	SensorDisplay::saveSettings(doc, element);

	if (save)
		setModified(false);

	return (true);
}

void
MultiMeter::configureSettings()
{
	mms = new MultiMeterSettings(this, "MultiMeterSettings");
	Q_CHECK_PTR(mms);
	mms->setTitle(title());
	mms->setShowUnit(showUnit());
	mms->setLowerLimitActive(lowerLimitActive);
	mms->setLowerLimit(lowerLimit);
	mms->setUpperLimitActive(upperLimitActive);
	mms->setUpperLimit(upperLimit);
	mms->setNormalDigitColor(normalDigitColor);
	mms->setAlarmDigitColor(alarmDigitColor);
	mms->setMeterBackgroundColor(lcd->backgroundColor());

	connect(mms, SIGNAL(applyClicked()), SLOT(applySettings()));

	if (mms->exec())
		applySettings();

	delete mms;
	mms = 0;
}

void
MultiMeter::applySettings()
{
	setShowUnit( mms->showUnit() );
	setTitle(mms->title());
	lowerLimitActive = mms->lowerLimitActive();
	lowerLimit = mms->lowerLimit();
	upperLimitActive = mms->upperLimitActive();
	upperLimit = mms->upperLimit();

	normalDigitColor = mms->normalDigitColor();
	alarmDigitColor = mms->alarmDigitColor();
	setBackgroundColor(mms->meterBackgroundColor());

	repaint();
	setModified(true);
}

void
MultiMeter::applyStyle()
{
	normalDigitColor = KSGRD::Style->firstForegroundColor();
	setBackgroundColor(KSGRD::Style->backgroundColor());
	repaint();
	setModified(true);
}

void
MultiMeter::setDigitColor(const QColor& col)
{
	QPalette p = lcd->palette();
	p.setColor(QPalette::Foreground, col);
	lcd->setPalette(p);
}

void
MultiMeter::setBackgroundColor(const QColor& col)
{
	lcd->setBackgroundColor(col);

	QPalette p = lcd->palette();
	p.setColor(QPalette::Light, col);
	p.setColor(QPalette::Dark, col);
	lcd->setPalette(p);
}
