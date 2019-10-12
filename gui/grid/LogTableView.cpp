#include "LogTableView.h"
#include "LogTable.h"
#include "seer/Log.h"
#include <QFontMetricsF>
#include <QPaintEvent>
#include <QPainter>
#include <QScrollBar>
#include <QAction>
#include <QGuiApplication>
#include <QApplication>
#include <QClipboard>
#include <QMenu>
#include <cmath>

namespace gui::grid {

    void LogTableView::paintRow(QPainter* painter, int row, int y) {
        auto model = _table->model();
        auto columns = _table->header()->count();

        if (model->isSelected(row)) {
            QBrush b(palette().color(QPalette::Highlight));
            QRect r(0, y, width(), _rowHeight);
            painter->fillRect(r, b);
            painter->setPen(palette().color(QPalette::HighlightedText));
        } else {
            auto index = model->index(row, 0);
            assert(index.isValid());
            auto brush = model->data(index, Qt::ForegroundRole).value<QBrush>();
            painter->setPen(brush.color());
        }

        QFontMetricsF fm(font());

        for (auto column = 0; column < columns; ++column) {
            auto x = _table->header()->sectionPosition(column);
            assert(row < model->rowCount({}));
            assert(column < model->columnCount({}));
            auto index = model->index(row, column);
            assert(index.isValid());
            auto text = model->data(index, Qt::DisplayRole).toString();
            auto sectionSize = _table->header()->sectionSize(column);

            auto textWidth = [&](QString const& text) {
                int index = 0;
                const auto& split = text.split("\t");
                for (int i = 0; i < split.size(); ++i) {
                    index += std::round(fm.width(split[i]) / _charWidth);
                    if (i != split.size() - 1) {
                        index += _tabWidth - (index % _tabWidth);
                    }
                }
                return index * _charWidth;
            };

            auto isMessageColumn = column == columns - 1;
            float left = 0;
            if (_searcher && !model->isSelected(row) && (isMessageColumn || !_messageOnlyHighlight)) {
                int currentIndex = 0;
                for (;;) {
                    auto [first, len] = _searcher->search(text, currentIndex);
                    if (first == -1)
                        break;
                    QRect r;
                    left += textWidth(text.mid(currentIndex, first - currentIndex));
                    if (left >= sectionSize)
                        break;
                    r.setLeft(x + left);
                    left = std::min<float>(sectionSize, left + textWidth(text.mid(first, len)));
                    r.setRight(x + left);
                    r.setTop(y);
                    r.setBottom(y + _rowHeight);
                    painter->fillRect(r, QBrush(QColor::fromRgb(0xfb, 0xfa, 0x08)));
                    currentIndex = first + len;
                }
            }

            QRectF rect;
            rect.setLeft(x);
            rect.setRight(std::min(x + sectionSize,
                                   _table->scrollArea()->horizontalScrollBar()->value() + _table->width()));
            rect.setTop(y);
            rect.setBottom(y + _rowHeight);
            QTextOption option;
            option.setTabStopDistance(_charWidth * _tabWidth);
            option.setWrapMode(QTextOption::WrapAnywhere);
            painter->drawText(rect, text, option);
        }
    }

    int LogTableView::getRow(int y) {
        auto model = _table->model();
        if (!model)
            return -1;
        y = y + _table->scrollArea()->y() - _table->header()->height();
        return y / _rowHeight + _firstRow;
    }

    void LogTableView::copyToClipboard(bool raw) {
        auto model = _table->model();
        if (!model)
            return;
        auto [first, last] = model->getSelection();
        if (first == -1)
            return;
        seer::log_infof("copying to clipboard lines [%d; %d)", first, last);
        QString text;
        auto append =  [&] (auto& line) {
            text += QString::fromStdString(line);
            text += "\n";
        };
        if (raw) {
            model->copyRawLines(first, last, append);
        } else {
            model->copyLines(first, last, append);
        }
        assert(text.size());
        text.resize(text.size() - 1);
        QGuiApplication::clipboard()->setText(text);
    }

    LogTableView::LogTableView(QFont font, LogTable* parent) : QWidget(parent), _table(parent) {
        setFocusPolicy(Qt::ClickFocus);
        setFont(font);
        QFontMetricsF fm(font);
        _rowHeight = fm.height();
        _charWidth = fm.width(' ');
        setMouseTracking(true);

        auto copyAction = new QAction("Copy selected lines", _table->scrollArea());
        copyAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_C));
        copyAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(copyAction, &QAction::triggered, [=] { LogTableView::copyToClipboard(true); });
        addAction(copyAction);

        auto copyFormattedAction = new QAction("Copy selected lines (formatted)", _table->scrollArea());
        copyFormattedAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C));
        copyFormattedAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(copyFormattedAction, &QAction::triggered, [=] { LogTableView::copyToClipboard(false); });
        addAction(copyFormattedAction);

        setContextMenuPolicy(Qt::CustomContextMenu);
        connect(this, &QWidget::customContextMenuRequested, [=] (const QPoint& pos) {
            QMenu menu(this);
            menu.addAction(copyAction);
            menu.addAction(copyFormattedAction);
            menu.exec(mapToGlobal(pos));
        });
    }

    void LogTableView::paintEvent(QPaintEvent* event) {
        auto model = _table->model();
        if (!model)
            return;

        QPainter painter(this);
        painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

        int row = _firstRow;
        int y = 0;

        int maxY = event->rect().height() + _rowHeight;
        for (; row < _table->model()->rowCount(QModelIndex()); ++row) {
            painter.save();
            paintRow(&painter, row, y);
            painter.restore();
            y += _rowHeight;
            if (y > maxY)
                break;
        }
    }

    void LogTableView::mousePressEvent(QMouseEvent *event) {
        if (event->button() != Qt::LeftButton)
            return;
        auto row = getRow(event->y());
        if (row == -1)
            return;
        if (row < _table->model()->rowCount({})) {
            if (QApplication::keyboardModifiers() & Qt::ShiftModifier) {
                _table->model()->extendSelection(row);
            } else {
                _table->model()->setSelection(row);
            }
        }
    }

    void LogTableView::mouseMoveEvent(QMouseEvent *event) {
        if (!(event->buttons() & Qt::LeftButton))
            return;
        auto row = getRow(event->y());
        if (row == -1)
            return;
        _table->model()->extendSelection(row);
        update();
    }

    void LogTableView::setFirstRow(int row) {
        _firstRow = row;
    }

    int LogTableView::visibleRows() {
        return height() / _rowHeight;
    }

    float LogTableView::charWidth() const {
        return _charWidth;
    }

    void LogTableView::setSearchHighlight(std::string text,
                                          bool regex,
                                          bool caseSensitive,
                                          bool messageOnly) {
        _messageOnlyHighlight = messageOnly;
        auto qText = QString::fromStdString(text);
        _searcher = seer::createSearcher(qText, regex, caseSensitive);
        update();
    }

} // namespace gui::grid
