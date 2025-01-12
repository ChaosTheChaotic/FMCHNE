#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <vector>
#include <string>
#include <QPushButton>
#include <QLabel>
#include "RotatableButton.h"
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onButtonPressed();
    void onButtonReleased();
    void onSpinButtonClicked();

private:
    void setupStart();
    void initializeScreenDimensions();
    void clearScreen(int screenId);
    void gameScreen();
    void onClaimButtonClicked();
    void updateMoneyLabel();
    void endScreen();
    bool hasThreeOfAKind(const std::vector<std::string>& results) const;
    bool hasTwoOfAKind(const std::vector<std::string>& results) const;
    int countSymbol(const std::vector<std::string>& results, const std::string& symbol) const; 
    std::vector<std::string> generateRandomSymbol() const;
    void setupButton(QPushButton* button, const QString& styleSheet);
    QString getDefaultButtonStyle() const;
    QString getHoverButtonStyle() const;
    void saveState();
    void readSave();
    bool hasSaveFile() const;
    void removeSaveState();

    std::unique_ptr<Ui::MainWindow> ui;
    QLabel* m_moneyLabel = nullptr;
    
    // Game state
    int m_money{100};
    int m_cost{20};
    std::vector<std::string> m_symbols{"Cherry", "Bell", "Lemon", "Orange", "Star", "Skull"};
    int m_spinCount{0};
    int m_maxMoney{100}; // Tracks highest amount of money held
    int m_highestSpin{0};
    int m_totalSpins{0};
    int m_totalMoneyEarnt{0};
    int m_allTimeHighestMoney{0};
    int m_previousMoney{0};
    int m_runsPlayed{0};
    // Layout
    QRect m_screenGeometry;
    QPoint m_screenCenter;
    QPoint m_originalButtonPosition;
    QLabel* m_reelLabels[3] = {nullptr, nullptr, nullptr};  // Array to hold the three reel labels
    RotatableButton* m_spinButton{nullptr};
    RotatableButton* m_claimButton{nullptr};
    RotatableButton* m_exitButton{nullptr};

   // Constants
    static constexpr int BUTTON_WIDTH = 180;
    static constexpr int BUTTON_HEIGHT = 60;
    static constexpr int TITLE_HEIGHT = 100;
    static constexpr int REEL_WIDTH = 150;   // Width of each reel
    static constexpr int REEL_HEIGHT = 150;  // Height of each reel
    static constexpr int REEL_SPACING = 20;  // Space between reels
    static constexpr int REELS_CONTAINER_WIDTH = (REEL_WIDTH * 3) + (REEL_SPACING * 2);
    static constexpr int REELS_CONTAINER_HEIGHT = REEL_HEIGHT;
    const QString SAVE_FILE = "game_save.json";
};
#endif // MAINWINDOW_H
