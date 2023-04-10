/*
   Copyright (C) 2001-2006, William Joseph.
   All Rights Reserved.

   This file is part of GtkRadiant.

   GtkRadiant is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   GtkRadiant is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GtkRadiant; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once

#include <QComboBox>
#include <QKeyEvent>

/// @brief Subclassed QComboBox not comsuming Enter key (why does it do it? works as expected for editable ComboBox)
/// purpose is to have working confirmation by Enter in dialogs
/// fixme unsolved crude problem here is triggering arrows, page, home, end global shortcuts when pressed in popup; even if modal dialog 😱
class ComboBox : public QComboBox
{
protected:
	void keyPressEvent( QKeyEvent *event ) override {
		if( event->key() == Qt::Key_Enter
		 || event->key() == Qt::Key_Return ){
			event->ignore();
			return;
		}
		QComboBox::keyPressEvent( event );
	}
};
