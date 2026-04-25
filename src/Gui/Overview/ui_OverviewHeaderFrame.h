/********************************************************************************
** Form generated from reading UI file 'OverviewHeaderFrame.ui'
**
** Created by: Qt User Interface Compiler version 5.15.x
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_OVERVIEWHEADERFRAME_H
#define UI_OVERVIEWHEADERFRAME_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_OverviewHeaderFrame
{
public:
    QHBoxLayout *m_rootLayout;

    void setupUi(QFrame *OverviewHeaderFrame)
    {
        if (OverviewHeaderFrame->objectName().isEmpty())
            OverviewHeaderFrame->setObjectName(QStringLiteral("OverviewHeaderFrame"));
        OverviewHeaderFrame->resize(1068, 100);
        OverviewHeaderFrame->setFrameShape(QFrame::NoFrame);
        OverviewHeaderFrame->setFrameShadow(QFrame::Raised);
        m_rootLayout = new QHBoxLayout(OverviewHeaderFrame);
        m_rootLayout->setSpacing(0);
        m_rootLayout->setContentsMargins(0, 0, 0, 0);
        m_rootLayout->setObjectName(QStringLiteral("m_rootLayout"));

        retranslateUi(OverviewHeaderFrame);

        QMetaObject::connectSlotsByName(OverviewHeaderFrame);
    } // setupUi

    void retranslateUi(QFrame *OverviewHeaderFrame)
    {
        OverviewHeaderFrame->setWindowTitle(QApplication::translate("OverviewHeaderFrame", "Frame", nullptr));
    } // retranslateUi

};

namespace Ui {
    class OverviewHeaderFrame: public Ui_OverviewHeaderFrame {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_OVERVIEWHEADERFRAME_H
