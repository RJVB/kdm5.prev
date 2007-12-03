/*
 *  Copyright (C) 2003 by Unai Garro <ugarro@users.sourceforge.net>
 *  Copyright (C) 2004 by Enrico Ros <rosenric@dei.unipd.it>
 *  Copyright (C) 2004 by Stephan Kulow <coolo@kde.org>
 *  Copyright (C) 2004 by Oswald Buddenhagen <ossi@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "kdmpixmap.h"
#include "kdmthemer.h"

#include <ksvgrenderer.h>

#include <QPainter>
#include <QSignalMapper>

KdmPixmap::KdmPixmap( QObject *parent, const QDomNode &node )
	: KdmItem( parent, node )
	, qsm( 0 )
{
	itemType = "pixmap";
	if (!isVisible())
		return;

	// Set default values for pixmap (note: strings are already Null)
	pixmap.normal.tint.setRgb( 0xFFFFFF );
	pixmap.normal.svgRenderer = 0;
	pixmap.active.present = false;
	pixmap.active.svgRenderer = 0;
	pixmap.prelight.present = false;
	pixmap.prelight.svgRenderer = 0;

	// Read PIXMAP TAGS
	QDomNodeList childList = node.childNodes();
	for (int nod = 0; nod < childList.count(); nod++) {
		QDomNode child = childList.item( nod );
		QDomElement el = child.toElement();
		QString tagName = el.tagName();

		if (tagName == "normal") {
			definePixmap( el, pixmap.normal );
			parseColor( el.attribute( "tint", "#ffffff" ), el.attribute( "alpha", "1.0" ), pixmap.normal.tint );
		} else if (tagName == "active") {
			pixmap.active.present = true;
			definePixmap( el, pixmap.active );
			parseColor( el.attribute( "tint", "#ffffff" ), el.attribute( "alpha", "1.0" ), pixmap.active.tint );
		} else if (tagName == "prelight") {
			pixmap.prelight.present = true;
			definePixmap( el, pixmap.prelight );
			parseColor( el.attribute( "tint", "#ffffff" ), el.attribute( "alpha", "1.0" ), pixmap.prelight.tint );
		}
	}
}

void
KdmPixmap::definePixmap( const QDomElement &el, PixmapStruct::PixmapClass &pClass )
{
	QString fileName = el.attribute( "file" );
	if (fileName.isEmpty())
		return;

	pClass.fullpath = fileName;
	if (fileName.at( 0 ) != '/')
		pClass.fullpath = themer()->baseDir() + '/' + fileName;

	pClass.svgImage = fileName.endsWith( ".svg" ) || fileName.endsWith( ".svgz" );
}

bool
KdmPixmap::loadPixmap( PixmapStruct::PixmapClass &pClass )
{
	if (!pClass.image.isNull())
		return true;
	if (pClass.fullpath.isEmpty())
		return false;
	if (area.isValid()) {
		int dot = pClass.fullpath.lastIndexOf( '.' );
		if (pClass.image.load( pClass.fullpath.left( dot )
				.append( QString( "-%1x%2" )
					.arg( area.width() ).arg( area.height() ) )
				.append( pClass.fullpath.mid( dot ) ) ))
			goto gotit;
	}
	if (!pClass.image.load( pClass.fullpath )) {
		kWarning() << "failed to load " << pClass.fullpath ;
		pClass.fullpath.clear();
		return false;
	}
  gotit:
	if (pClass.image.format() != QImage::Format_ARGB32)
		pClass.image = pClass.image.convertToFormat( QImage::Format_ARGB32 );
	applyTint( pClass, pClass.image );
	return true;
}

bool
KdmPixmap::loadSvg( PixmapStruct::PixmapClass &pClass )
{
	if (pClass.svgRenderer)
		return true;
	if (pClass.fullpath.isEmpty())
		return false;
	pClass.svgRenderer = new KSvgRenderer( pClass.fullpath, this );
	if (!pClass.svgRenderer->isValid()) {
		delete pClass.svgRenderer;
		pClass.svgRenderer = 0;
		kWarning() << "failed to load " << pClass.fullpath ;
		pClass.fullpath.clear();
		return false;
	}
	if (pClass.svgRenderer->animated()) {
		if (!qsm) {
			qsm = new QSignalMapper( this );
			connect( qsm, SIGNAL(mapped( int )), SLOT(slotAnimate( int )) );
		}
		qsm->setMapping( pClass.svgRenderer, state ); // assuming we only load the current state
		connect( pClass.svgRenderer, SIGNAL(repaintNeeded()), qsm, SLOT(map()) );
	}
	return true;
}

QSize
KdmPixmap::sizeHint()
{
	// use the pixmap size as the size hint
	if (!pixmap.normal.svgImage && loadPixmap( pixmap.normal ))
		return pixmap.normal.image.size();
	return KdmItem::sizeHint();
}

void
KdmPixmap::updateSize( PixmapStruct::PixmapClass &pClass )
{
	if (pClass.readyPixmap.size() != area.size())
		pClass.readyPixmap = QPixmap();
}

void
KdmPixmap::setGeometry( QStack<QSize> &parentSizes, const QRect &newGeometry, bool force )
{
	KdmItem::setGeometry( parentSizes, newGeometry, force );
	updateSize( pixmap.active );
	updateSize( pixmap.prelight );
	updateSize( pixmap.normal );
}


void
KdmPixmap::drawContents( QPainter *p, const QRect &r )
{
	PixmapStruct::PixmapClass &pClass = getCurClass();

	int px = area.left() + r.left();
	int py = area.top() + r.top();
	int sx = r.x();
	int sy = r.y();
	int sw = r.width();
	int sh = r.height();
	if (px < 0) {
		px *= -1;
		sx += px;
		px = 0;
	}
	if (py < 0) {
		py *= -1;
		sy += py;
		py = 0;
	}


	if (pClass.readyPixmap.isNull()) {
		QImage scaledImage;

		if (pClass.svgImage) {
			if (loadSvg( pClass )) {
				scaledImage = QImage( area.size(), QImage::Format_ARGB32 );
				scaledImage.fill( 0 );
				QPainter pa( &scaledImage );
				pClass.svgRenderer->render( &pa );
				applyTint( pClass, scaledImage );
			}
		} else {
			// use the loaded pixmap or a scaled version if needed
			if (area.size() != pClass.image.size()) { // true for isNull
				if (loadPixmap( pClass ))
					scaledImage = pClass.image.scaled( area.size(),
						Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
			} else
				scaledImage = pClass.image;
		}

		if (scaledImage.isNull()) {
			p->fillRect( px, py, sw, sh, Qt::black );
			return;
		}

		pClass.readyPixmap = QPixmap::fromImage( scaledImage );
	}
	// kDebug() << "Pixmap::drawContents " << pClass.readyPixmap.size() << " " << px << " " << py << " " << sx << " " << sy << " " << sw << " " << sh;
	p->drawPixmap( px, py, pClass.readyPixmap, sx, sy, sw, sh );
}

void
KdmPixmap::applyTint( PixmapStruct::PixmapClass &pClass, QImage &img )
{
	if (pClass.tint.rgba() == 0xFFFFFFFF)
		return;

	int w = img.width();
	int h = img.height();
	int tint_red = pClass.tint.red();
	int tint_green = pClass.tint.green();
	int tint_blue = pClass.tint.blue();
	int tint_alpha = pClass.tint.alpha();

	for (int y = 0; y < h; ++y) {
		QRgb *ls = (QRgb *)img.scanLine( y );
		for (int x = 0; x < w; ++x) {
			QRgb l = ls[x];
			int r = qRed( l ) * tint_red / 255;
			int g = qGreen( l ) * tint_green / 255;
			int b = qBlue( l ) * tint_blue / 255;
			int a = qAlpha( l ) * tint_alpha / 255;
			ls[x] = qRgba( r, g, b, a );
		}
	}
}

KdmPixmap::PixmapStruct::PixmapClass &
KdmPixmap::getClass( ItemState sts )
{
	return
		(sts == Sprelight && pixmap.prelight.present) ?
			pixmap.active :
			(sts == Sactive && pixmap.active.present) ?
				pixmap.active :
				pixmap.normal;
}

void
KdmPixmap::slotAnimate( int sts )
{
	PixmapStruct::PixmapClass &pClass = getClass( ItemState(sts) );
	pClass.readyPixmap = QPixmap();
	if (&pClass == &getCurClass())
		needUpdate();
}

void
KdmPixmap::statusChanged( bool descend )
{
	KdmItem::statusChanged( descend );
	if (!pixmap.active.present && !pixmap.prelight.present)
		return;
	if ((state == Sprelight && !pixmap.prelight.present) ||
	    (state == Sactive && !pixmap.active.present))
		return;
	needUpdate();
}

#include "kdmpixmap.moc"
