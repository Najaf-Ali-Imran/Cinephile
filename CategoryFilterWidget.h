#ifndef CATEGORYFILTERWIDGET_H
#define CATEGORYFILTERWIDGET_H

#include <QMap>
#include <QNetworkAccessManager>
#include <QWidget>
#include "MovieCard.h"
#include "Theme.h"

class QVBoxLayout;
class QGridLayout;
class QLabel;
class QScrollArea;
class QComboBox;
class QPushButton;
class QSpinBox;
class QCheckBox;
class QDateEdit;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QHBoxLayout;
class QFormLayout;
class QResizeEvent;
class QNetworkReply;

class CategoryFilterWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CategoryFilterWidget(QNetworkAccessManager *networkManager, QWidget *parent = nullptr);
    ~CategoryFilterWidget();

    void loadFilters();

signals:
    void movieClicked(int id, const QString &type);

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void applyFilters(bool reset = true);
    void processMoviesReply(QNetworkReply *reply);
    void fetchGenres(const QString &type);
    void processGenreReply(QNetworkReply *reply);
    void onScroll();
    void fetchLanguages();
    void processLanguagesReply(QNetworkReply *reply);
    void fetchRegions();
    void processRegionsReply(QNetworkReply *reply);
    void clearFilters();
    void toggleFilterVisibility();
    void clearListWidgetSelection(QListWidget *listWidget);

private:
    QVBoxLayout *m_contentVBoxLayout;
    QScrollArea *m_scrollArea;
    QWidget *m_scrollContent;
    QGridLayout *m_gridLayout;
    QFormLayout *m_filterFormLayout;
    QLabel *m_statusLabel;
    QPushButton *m_filterToggleButton;
    QWidget *m_filterContainerWidget;

    QComboBox *m_typeCombo;
    QComboBox *m_sortCombo;
    QComboBox *m_yearCombo;
    QListWidget *m_genreListWidget;
    QSpinBox *m_minVoteAverageSpinBox;
    QSpinBox *m_maxVoteAverageSpinBox;
    QSpinBox *m_minVoteCountSpinBox;
    QSpinBox *m_maxVoteCountSpinBox;
    QListWidget *m_languageListWidget;
    QListWidget *m_regionListWidget;
    QDateEdit *m_releaseDateFromEdit;
    QDateEdit *m_releaseDateToEdit;
    QSpinBox *m_minRuntimeSpinBox;
    QSpinBox *m_maxRuntimeSpinBox;
    QCheckBox *m_includeAdultCheckBox;
    QLineEdit *m_keywordsLineEdit;
    QPushButton *m_applyButton;
    QPushButton *m_clearButton;

    QNetworkAccessManager *m_networkManager;
    QMap<QString, QString> m_genreMap;
    QMap<QString, QString> m_languageMap;
    QMap<QString, QString> m_regionMap;
    QMap<QString, QString> m_sortOptionsMap;
    QList<MovieCard *> m_movieCards;

    QString m_currentType;
    int m_currentPage = 1;
    bool m_fetching = false;
    int m_gridColumns = 5;
    bool m_filtersVisible = false;

    void setupUI();
    void clearGrid(bool deleteWidgets = true);
    void resetPagination();
    void relayoutGrid();
    void showStatusMessage(const QString &message,
                           const QString &color = AppTheme::TEXT_ON_DARK_SECONDARY);
    QStringList selectedGenreIds() const;
    QStringList selectedLanguageCodes() const;
    QStringList selectedRegionCodes() const;
};

#endif
