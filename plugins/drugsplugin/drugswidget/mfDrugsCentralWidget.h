/***************************************************************************
 *   FreeMedicalForms                                                      *
 *   Copyright (C) 2008-2009 by Eric MAEKER                                *
 *   eric.maeker@free.fr                                                   *
 *   All rights reserved.                                                  *
 *                                                                         *
 *   This program is a free and open source software.                      *
 *   It is released under the terms of the new BSD License.                *
 *                                                                         *
 *   Redistribution and use in source and binary forms, with or without    *
 *   modification, are permitted provided that the following conditions    *
 *   are met:                                                              *
 *   - Redistributions of source code must retain the above copyright      *
 *   notice, this list of conditions and the following disclaimer.         *
 *   - Redistributions in binary form must reproduce the above copyright   *
 *   notice, this list of conditions and the following disclaimer in the   *
 *   documentation and/or other materials provided with the distribution.  *
 *   - Neither the name of the FreeMedForms' organization nor the names of *
 *   its contributors may be used to endorse or promote products derived   *
 *   from this software without specific prior written permission.         *
 *                                                                         *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS   *
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT     *
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS     *
 *   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE        *
 *   COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,  *
 *   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,  *
 *   BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;      *
 *   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER      *
 *   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT    *
 *   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN     *
 *   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE       *
 *   POSSIBILITY OF SUCH DAMAGE.                                           *
 ***************************************************************************/
/***************************************************************************
 *   Main Developper : Eric MAEKER, <eric.maeker@free.fr>                  *
 *   Contributors :                                                        *
 *       NAME <MAIL@ADRESS>                                                *
 *       NAME <MAIL@ADRESS>                                                *
 ***************************************************************************/
#ifndef MFDRUGSCENTRALWIDGET_H
#define MFDRUGSCENTRALWIDGET_H

#include <QWidget>
#include <QObject>
#include <QListView>

/**
 * \file mfDrugsCentralWidget.h
 * \author Eric MAEKER <eric.maeker@free.fr>
 * \version 0.0.4
 * \date 15 Sept 2009
 * \brief Includes in the same widget : drugselector, prescriptionviewer. Connections are made easy.
   \ingroup freediams
*/


namespace Drugs {
namespace Internal {
class DrugsContext;
class PrescriptionViewer;
class DrugsActionHandler;
class DrugsModel;
namespace Ui {
class DrugsCentralWidget;
}  // End Ui
}  // End Internal

class DrugsCentralWidget : public QWidget
{
    Q_OBJECT
    friend class Drugs::Internal::DrugsActionHandler;

public:
    DrugsCentralWidget(QWidget *parent = 0);
    bool initialize();

    void changeFontTo(const QFont &font);
    Internal::DrugsModel *currentDrugsModel() const;

    QListView *prescriptionListView();
    Internal::PrescriptionViewer *prescriptionView();

    void setCurrentSearchMethod(int method);
    bool printPrescription();

protected:
    void createConnections();
    void disconnect();

private Q_SLOTS:
    // drugs slots
    void selector_drugSelected( const int CIS );

private:
    void focusInEvent(QFocusEvent *event);

private:
    Internal::Ui::DrugsCentralWidget *m_ui;
    Internal::DrugsModel   *m_CurrentDrugModel;
    Internal::DrugsContext *m_Context;
};

}  // End Drugs

#endif // MFDRUGSCENTRALWIDGET_H
