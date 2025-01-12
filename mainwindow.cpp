#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "RotatableButton.h"

#include <QLabel>
#include <QScreen>
#include <QApplication>
#include <QPushButton>
#include <QGraphicsDropShadowEffect>
#include <random>
#include <QString>
#include <QRect>
#include <QPropertyAnimation>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QIODevice>
#include <QMap>
#include <QPixmap>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(std::make_unique<Ui::MainWindow>())
{
    ui->setupUi(this);
    initializeScreenDimensions();
    setupStart();
}

MainWindow::~MainWindow() = default;

void MainWindow::initializeScreenDimensions() {
    if (QScreen* screen = QApplication::primaryScreen()) {
        m_screenGeometry = screen->availableGeometry();
        m_screenCenter = m_screenGeometry.center();
    }
}

void MainWindow::saveState() {
    QJsonObject saveData;
    
    // Current game state
    QJsonObject current;
    current["Money"] = m_money;
    current["Spins"] = m_spinCount;
    current["MaxMoney"] = m_maxMoney;
    
    // Overall statistics
    QJsonObject overall;
    overall["TotalSpins"] = m_totalSpins;
    overall["TotalMoneyEarnt"] = m_totalMoneyEarnt;
    overall["HighestSpin"] = m_highestSpin;
    overall["AllTimeHighestMoney"] = m_allTimeHighestMoney;
    overall["Runs"] = m_runsPlayed;
    
    // Combine both into main object
    saveData["Current"] = current;
    saveData["Overall"] = overall;

    QJsonDocument doc(saveData);
    QFile file(SAVE_FILE);

    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        qDebug() << "Game state saved successfully";
    } else {
        qDebug() << "Failed to save game state";
    }
}

void MainWindow::readSave() {
    QFile file(SAVE_FILE);
    
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray saveData = file.readAll();
        QJsonDocument doc(QJsonDocument::fromJson(saveData));
        QJsonObject saveObj = doc.object();
        
        // Load current game state
        QJsonObject current = saveObj["Current"].toObject();
        m_money = current["Money"].toInt();
        m_spinCount = current["Spins"].toInt();
        m_maxMoney = current["MaxMoney"].toInt();
        
        // Load overall statistics
        QJsonObject overall = saveObj["Overall"].toObject();
        m_totalSpins = overall["TotalSpins"].toInt();
        m_totalMoneyEarnt = overall["TotalMoneyEarnt"].toInt();
        m_highestSpin = overall["HighestSpin"].toInt();
        m_allTimeHighestMoney = overall["AllTimeHighestMoney"].toInt();
	m_runsPlayed = overall["Runs"].toInt();
        
        file.close();
        qDebug() << "Game state loaded successfully";
    }
}

void MainWindow::removeSaveState() {
    QFile file(SAVE_FILE);
    
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray saveData = file.readAll();
        QJsonDocument doc(QJsonDocument::fromJson(saveData));
        QJsonObject saveObj = doc.object();
        
        // Remove only the current game state
        saveObj.remove("Current");
        
        file.close();
        
        // Write back the file with only overall stats
        if (file.open(QIODevice::WriteOnly)) {
            QJsonDocument newDoc(saveObj);
            file.write(newDoc.toJson());
            file.close();
        }
    }
}

std::vector<std::string> MainWindow::generateRandomSymbol() const {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, m_symbols.size() - 1);
    
    std::vector<std::string> rolled;
    rolled.reserve(3);
    
    // Generate 3 random symbols
    for (int i = 0; i < 3; ++i) {
        int randomIndex = dis(gen);
        rolled.push_back(m_symbols[randomIndex]);
    }
    
    return rolled;
}

void MainWindow::updateMoneyLabel() {
    if (m_moneyLabel) {
        m_maxMoney = std::max(m_maxMoney, m_money);
        
        int pounds = m_money / 100;
        int pence = m_money % 100;
        m_moneyLabel->setText(QString("Balance: £%1.%2")
            .arg(pounds)
            .arg(pence, 2, 10, QChar('0')));

        if (m_money < m_cost) {
            removeSaveState();
            clearScreen(3);
        }
    }
}

bool MainWindow::hasSaveFile() const {
    QFile file(SAVE_FILE);
    if (file.exists()) {
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            QJsonObject obj = doc.object();
            bool hasContent = obj.contains("Current");
            file.close();
            return hasContent;
        }
    }
    return false;
}

bool MainWindow::hasThreeOfAKind(const std::vector<std::string>& results) const {
    return results[0] == results[1] && results[1] == results[2];
}

bool MainWindow::hasTwoOfAKind(const std::vector<std::string>& results) const {
    return (results[0] == results[1] || 
            results[1] == results[2] || 
            results[0] == results[2]);
}

int MainWindow::countSymbol(const std::vector<std::string>& results, const std::string& symbol) const {
    return std::count(results.begin(), results.end(), symbol);
}

QString MainWindow::getDefaultButtonStyle() const {
    return QString(
        "QPushButton {"
        "   background-color: qlineargradient(y1: 0, y2: 1, stop: 0 #FFD700, stop: 1 #FF8C00);"
        "   color: black;"
        "   border-radius: 10px;"
        "   font-size: 20px;"
        "   font-weight: bold;"
        "   padding: 0px;"
        "   border: 2px solid #FFD700;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #FF4500;"
        "   color: white;"
        "}"
    );
}

QString MainWindow::getHoverButtonStyle() const {
    return QString(
        "QPushButton {"
        "   background-color: #FF6347;"
        "   border-radius: 10px;"
        "   font-size: 20px;"
        "   font-weight: bold;"
        "   padding: 10px;"
        "   border: 2px solid #FFD700;"
        "}"
    );
}

void MainWindow::setupButton(QPushButton* button, const QString& styleSheet) {
    if (!button) return;
    
    if (auto* rotButton = qobject_cast<RotatableButton*>(button)) {
        // For RotatableButton, set the visual button size
        rotButton->setButtonSize(QSize(BUTTON_WIDTH, BUTTON_HEIGHT));
    } else {
        // For regular QPushButton
        button->setFixedSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    }
    
    button->setStyleSheet(styleSheet);
    button->installEventFilter(this);
}


void MainWindow::onButtonPressed() {
    if (auto* button = qobject_cast<QPushButton*>(sender())) {
        QPoint fixedPosition = button->pos();

        auto* scaleAnim = new QPropertyAnimation(button, "size", this);
        scaleAnim->setDuration(100);
        
        QSize startSize = button->size();
        QSize endSize(startSize.width() * 0.9, startSize.height() * 0.9);
        
        scaleAnim->setStartValue(startSize);
        scaleAnim->setEndValue(endSize);
        
        // Ensure position stays fixed during animation
        connect(scaleAnim, &QPropertyAnimation::valueChanged, [button, fixedPosition]() {
            button->move(fixedPosition);
        });
        
        scaleAnim->start(QAbstractAnimation::DeleteWhenStopped);

        // Update the visual style
        button->setStyleSheet(
            "QPushButton {"
            "   background-color: #FF4500;"
            "   color: white;"
            "   border-radius: 10px;"
            "   font-size: 20px;"
            "   font-weight: bold;"
            "   padding: 0px;"
            "   border: 2px solid #FFD700;"
            "}"
        );
    }
}

void MainWindow::onButtonReleased() {
    if (auto* button = qobject_cast<QPushButton*>(sender())) {
        QPoint fixedPosition = button->pos();

        auto* scaleAnim = new QPropertyAnimation(button, "size", this);
        scaleAnim->setDuration(100);
        
        QSize currentSize = button->size();
        QSize originalSize(currentSize.width() / 0.9, currentSize.height() / 0.9);
        
        scaleAnim->setStartValue(currentSize);
        scaleAnim->setEndValue(originalSize);
        
        // Ensure position stays fixed during animation
        connect(scaleAnim, &QPropertyAnimation::valueChanged, [button, fixedPosition]() {
            button->move(fixedPosition);
        });
        
        scaleAnim->start(QAbstractAnimation::DeleteWhenStopped);

        // Restore default style
        button->setStyleSheet(getDefaultButtonStyle());
    }
}

void MainWindow::onClaimButtonClicked() {
    if (m_money > 0) {
        m_maxMoney = std::max(m_maxMoney, m_money);

        removeSaveState();
        clearScreen(3);
    }
}

void MainWindow::onSpinButtonClicked() {
    qDebug() << "Spin button clicked!";
    if (m_money >= m_cost) {
        m_spinCount++;
        m_totalSpins++;
        m_highestSpin = std::max(m_spinCount, m_highestSpin);
        m_previousMoney = m_money;
        m_money -= m_cost;
        updateMoneyLabel();

        std::vector<std::string> results = generateRandomSymbol();
        
        // Map of symbols to emojis
        const QMap<QString, QString> symbolToEmoji{
            {"Cherry", "🍒"},
            {"Bell", "🔔"},
            {"Lemon", "🍋"},
            {"Orange", "🍊"},
            {"Star", "⭐"},
            {"Skull", "💀"}
        };

        // Update the reel displays with emojis
        for (int i = 0; i < 3; ++i) {
            if (m_reelLabels[i]) {
                QString symbol = QString::fromStdString(results[i]);
                m_reelLabels[i]->setText(symbolToEmoji.value(symbol));
                qInfo() << symbol << "->" << symbolToEmoji.value(symbol);
            }
        }

        // Check for skulls first (losses)
        int skullCount = countSymbol(results, "Skull");
        if (skullCount >= 3) {
            m_money = 0;
            qInfo() << "Game Over - Three skulls!";
        } else if (skullCount == 2) {
            m_money = std::max(0, m_money - 100);
            qInfo() << "Lost £1 - Two skulls!";
        } else {
            // Check for wins
            if (hasThreeOfAKind(results)) {
                if (results[0] == "Bell") {
                    m_money += 500;
                    qInfo() << "Jackpot! Won £5!";
                } else {
                    m_money += 100;
                    qInfo() << "Won £1 - Three of a kind!";
                }
            } else if (hasTwoOfAKind(results) && countSymbol(results, "Skull") == 0) {
                m_money += 50;
                qInfo() << "Won 50p - Two of a kind!";
            }
        }

        updateMoneyLabel();
    } else {
        qInfo() << "Insufficient funds!";
    }
    m_allTimeHighestMoney = std::max(m_money, m_allTimeHighestMoney);
    if (m_money > m_previousMoney){
        m_totalMoneyEarnt += (m_money - m_previousMoney);
    }
    updateMoneyLabel();
    saveState();
}

void MainWindow::setupStart() {
    m_runsPlayed += 1;
    // Background setup
    auto* backgroundWidget = new QWidget(this);
    backgroundWidget->setGeometry(m_screenGeometry);
    backgroundWidget->setStyleSheet(
        "background-color: qlineargradient(y1: 0, y2: 1, "
        "stop: 0 #1a001a, stop: 0.5 #330033, stop: 1 #4d004d);"
        "border: 2px solid #FFD700;"
    );

    // Frame setup
    auto* frameWidget = new QWidget(backgroundWidget);
    frameWidget->setGeometry(m_screenGeometry.adjusted(50, 50, -50, -50));
    frameWidget->setStyleSheet(
        "background-color: qlineargradient(y1: 0, y2: 1, "
        "stop: 0 #8B0000, stop: 1 #FF4500);"
        "border-radius: 20px;"
        "border: 5px solid #FFD700;"
    );

    // Title setup
    auto* titleLabel = new QLabel("The Fruit Machine", backgroundWidget);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(48);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("color: #FFD700;");
    titleLabel->resize(m_screenGeometry.width(), TITLE_HEIGHT);
    titleLabel->move(0, 20);

    // Play button setup
    auto* playButton = new RotatableButton("Play!", backgroundWidget); // Changed parent
    setupButton(playButton, getDefaultButtonStyle());

    // Calculate button position relative to background widget
    m_originalButtonPosition = QPoint(
        (backgroundWidget->width() - BUTTON_WIDTH) / 2,
        (backgroundWidget->height() / 2) + 50
    );
    
    playButton->move(m_originalButtonPosition);

    // Connect button signals
    connect(playButton, &QPushButton::pressed, this, &MainWindow::onButtonPressed);
    connect(playButton, &QPushButton::released, this, &MainWindow::onButtonReleased);
    connect(playButton, &QPushButton::clicked, this, [this]() { // Changed from released to clicked
        clearScreen(1);
    });

    // Exit button setup
    m_exitButton = new RotatableButton("Exit", backgroundWidget);
    setupButton(m_exitButton, getDefaultButtonStyle());
    m_exitButton->move(
        (m_screenGeometry.width() - BUTTON_WIDTH) / 2,
        (m_screenGeometry.height() / 2) + 150  // Below claim button
    );

    connect(m_exitButton, &QPushButton::clicked, this, [this]() {
        QApplication::quit();
    });
    connect(m_exitButton, &QPushButton::pressed, this, &MainWindow::onButtonPressed);
    connect(m_exitButton, &QPushButton::released, this, &MainWindow::onButtonReleased);

   if (hasSaveFile()) {
        auto* continueButton = new RotatableButton("Continue", backgroundWidget);
        setupButton(continueButton, getDefaultButtonStyle());
        continueButton->move(
            (m_screenGeometry.width() - BUTTON_WIDTH) / 2,
            m_originalButtonPosition.y() + BUTTON_HEIGHT + 20
        );
        
        connect(continueButton, &QPushButton::pressed, this, &MainWindow::onButtonPressed);
        connect(continueButton, &QPushButton::released, this, &MainWindow::onButtonReleased);
        connect(continueButton, &QPushButton::clicked, this, [this]() {
            readSave();
            clearScreen(1);
        });
        continueButton->show();
        
        // Move exit button down if continue button exists
        m_exitButton->move(
            (m_screenGeometry.width() - BUTTON_WIDTH) / 2,
            m_originalButtonPosition.y() + (BUTTON_HEIGHT + 20) * 2
        );
    }

    // Image setup
    auto* imageLabel = new QLabel(backgroundWidget);
    QPixmap originalPixmap("./slots.png");  // Assuming image is in resources

    if (originalPixmap.isNull()) {
        qDebug() << "Failed to load image: slots.png";
    } else {
        // Calculate desired image size (e.g., 200x200 pixels)
        const int imageWidth = 200;
        const int imageHeight = 200;
    
        // Scale the pixmap while keeping aspect ratio
        QPixmap scaledPixmap = originalPixmap.scaled(
            imageWidth,
            imageHeight,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );
    
        imageLabel->setPixmap(scaledPixmap);
    
        // Calculate position to center the image
        int xPos = (m_screenGeometry.width() - scaledPixmap.width()) / 2;
    int yPos = (m_screenGeometry.height() - scaledPixmap.height()) / 2 - 50; // Adjust -50 as needed
    
        imageLabel->move(xPos, yPos);
        imageLabel->resize(scaledPixmap.size());
        imageLabel->show();
    }


     // Show everything
    backgroundWidget->show();
    frameWidget->show();
    titleLabel->show();
    playButton->show();
    m_exitButton->show();

}

void MainWindow::gameScreen() {
    // Background setup
    auto* backgroundWidget = new QWidget(this);
    backgroundWidget->setGeometry(m_screenGeometry);
    backgroundWidget->setStyleSheet(
        "background-color: qlineargradient(y1: 0, y2: 1, "
        "stop: 0 #1a001a, stop: 0.5 #330033, stop: 1 #4d004d);"
        "border: 2px solid #FFD700;"
    );

    // Frame setup
    auto* frameWidget = new QWidget(backgroundWidget);
    frameWidget->setGeometry(m_screenGeometry.adjusted(50, 50, -50, -50));
    frameWidget->setStyleSheet(
        "background-color: qlineargradient(y1: 0, y2: 1, "
        "stop: 0 #8B0000, stop: 1 #FF4500);"
        "border-radius: 20px;"
        "border: 5px solid #FFD700;"
    );

    // Title setup
    auto* titleLabel = new QLabel("The Fruit Machine", backgroundWidget);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(48);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("color: #FFD700;");
    titleLabel->resize(m_screenGeometry.width(), TITLE_HEIGHT);
    titleLabel->move(0, 20); 

    //Money setup
    m_moneyLabel = new QLabel(backgroundWidget);
    updateMoneyLabel();
    QFont moneyLabelFont("Arial", 32);
    moneyLabelFont.setBold(true);

    m_moneyLabel->setFont(moneyLabelFont);
    m_moneyLabel->setAlignment(Qt::AlignCenter);
    m_moneyLabel->setStyleSheet(
    "QLabel {"
    "   color: #FFD700;"
    "   background-color: rgba(0, 0, 0, 100);"
    "   border: 2px solid #FFD700;"
    "   border-radius: 15px;"
    "   padding: 10px 20px;"
    "   margin: 10px;"
    "}"
);

    // Calculate the label's width based on content
    QFontMetrics fm(moneyLabelFont);
    int textWidth = fm.horizontalAdvance(m_moneyLabel->text()) + 100;
    m_moneyLabel->setFixedWidth(textWidth);

    // Center the label
    m_moneyLabel->move(
        (m_screenGeometry.width() - textWidth) / 2,
        120
    );

  // Reels container setup
    auto* reelsWidget = new QWidget(backgroundWidget);
    reelsWidget->setGeometry(
        (m_screenGeometry.width() - REELS_CONTAINER_WIDTH) / 2,
        250,
        REELS_CONTAINER_WIDTH,
        REELS_CONTAINER_HEIGHT
    );
    reelsWidget->setStyleSheet(
        "background-color: #000000;"
        "border-radius: 10px;"
        "border: 3px solid #FFD700;"
   );

    // Create three separate reel slots
    for (int i = 0; i < 3; ++i) {
        m_reelLabels[i] = new QLabel(reelsWidget);
        m_reelLabels[i]->setFixedSize(REEL_WIDTH, REEL_HEIGHT);
        m_reelLabels[i]->setAlignment(Qt::AlignCenter);
        m_reelLabels[i]->setStyleSheet(
            "QLabel {"
            "    background-color: #1a1a1a;"
            "    border: 2px solid #FFD700;"
            "    border-radius: 5px;"
            "    color: #FFD700;"
            "    font-size: 48px;"
            "}"
        );
    
    // Calculate X position for each reel
    int xPos = i * (REEL_WIDTH + REEL_SPACING);
    int yPos = 0;
    
    m_reelLabels[i]->move(xPos, yPos);
    m_reelLabels[i]->setText("?");
}
    // Spin button setup (moved up)
    m_spinButton = new RotatableButton("Spin", backgroundWidget);
    setupButton(m_spinButton, getDefaultButtonStyle());
    m_spinButton->move(
        (m_screenGeometry.width() - BUTTON_WIDTH) / 2,
        500
	);

    // Claim button setup
    m_claimButton = new RotatableButton("Claim Winnings", backgroundWidget);
    setupButton(m_claimButton, getDefaultButtonStyle());
    m_claimButton->move(
        (m_screenGeometry.width() - BUTTON_WIDTH) / 2,
        570
    );

    connect(m_spinButton, &QPushButton::clicked, this, &MainWindow::onSpinButtonClicked);
    connect(m_spinButton, &QPushButton::pressed, this, &MainWindow::onButtonPressed);
    connect(m_spinButton, &QPushButton::released, this, &MainWindow::onButtonReleased);

    connect(m_claimButton, &QPushButton::clicked, this, &MainWindow::onClaimButtonClicked);
    connect(m_claimButton, &QPushButton::pressed, this, &MainWindow::onButtonPressed);
    connect(m_claimButton, &QPushButton::released, this, &MainWindow::onButtonReleased);

    // Show all widgets
    backgroundWidget->show();
    frameWidget->show();
    titleLabel->show();
    reelsWidget->show();
    m_spinButton->show();
    m_claimButton->show();
}

/*
screenIds:
0 - menu
1 - gameScreen
2 - endScreen
*/

void MainWindow::clearScreen(int screenId) {
    qInfo() << "Screen cleared!";
    
    // Store the screenId for after cleanup
    int nextScreen = screenId;
    
    // Schedule cleanup of existing widgets
    const auto children = findChildren<QWidget*>();
    for (auto* child : children) {
        if (child && child != ui->centralwidget) {
            child->deleteLater();
        }
    }

    /*
     Use a single-shot timer to ensure all widgets are properly cleaned up
     before creating new ones
    */
    QTimer::singleShot(0, this, [this, nextScreen]() {
        if (nextScreen == 0) {
            qInfo() << "Start screen!";
            setupStart();
        } else if (nextScreen == 1) {
            qInfo() << "Game started!";
            gameScreen();
        } else if (nextScreen == 3) {
            qInfo() << "End screen!";
            endScreen();
        }
    });
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
    auto* button = qobject_cast<QPushButton*>(watched);
    if (!button) {
        return QMainWindow::eventFilter(watched, event);
    }

    switch (event->type()) {
        case QEvent::Enter: {
            button->setStyleSheet(getHoverButtonStyle());

            auto* shadowEffect = new QGraphicsDropShadowEffect(this);
            shadowEffect->setOffset(0, 0);
            shadowEffect->setBlurRadius(10);
            button->setGraphicsEffect(shadowEffect);

            auto* rotationAnim = new QPropertyAnimation(button, "rotation");
            rotationAnim->setDuration(200);
            rotationAnim->setStartValue(0);
            rotationAnim->setEndValue(45);
            rotationAnim->start(QAbstractAnimation::DeleteWhenStopped);
            break;
        }
        
        case QEvent::Leave: {
            button->setStyleSheet(getDefaultButtonStyle());
            button->setGraphicsEffect(nullptr);

            auto* rotationAnim = new QPropertyAnimation(button, "rotation");
            rotationAnim->setDuration(200);
            rotationAnim->setStartValue(45);
            rotationAnim->setEndValue(0);
            rotationAnim->start(QAbstractAnimation::DeleteWhenStopped);
            break;
        }
        
        default:
            return QMainWindow::eventFilter(watched, event);
    }

    return true;
}

void MainWindow::endScreen() {
    // Background setup
    auto* backgroundWidget = new QWidget(this);
    backgroundWidget->setGeometry(m_screenGeometry);
    backgroundWidget->setStyleSheet(
        "background-color: qlineargradient(y1: 0, y2: 1, "
        "stop: 0 #1a001a, stop: 0.5 #330033, stop: 1 #4d004d);"
        "border: 2px solid #FFD700;"
    );

    // Frame setup
    auto* frameWidget = new QWidget(backgroundWidget);
    frameWidget->setGeometry(m_screenGeometry.adjusted(50, 50, -50, -50));
    frameWidget->setStyleSheet(
        "background-color: qlineargradient(y1: 0, y2: 1, "
        "stop: 0 #8B0000, stop: 1 #FF4500);"
        "border-radius: 20px;"
        "border: 5px solid #FFD700;"
    );

    // Set proper widget order first
    frameWidget->lower();
    backgroundWidget->lower();

    // Title setup
    auto* titleLabel = new QLabel("Game Over!", backgroundWidget);
    QFont titleFont;
    titleFont.setPointSize(48);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("color: #FFD700;");
    titleLabel->resize(m_screenGeometry.width(), TITLE_HEIGHT);
    titleLabel->move(0, 20);

    // Stats setup
    auto* statsLabel = new QLabel(backgroundWidget);
    statsLabel->setText(QString(
    "Current Game:\n"
    "Total Spins: %1\n"
    "Final Balance: £%2.%3\n"
    "Highest Balance: £%4.%5\n\n"
    "All Time Stats:\n"
    "Total Spins: %6\n"
    "Total Money Earned: £%7.%8\n"
    "Highest Spins in One Game: %9\n"
    "All-Time Highest Balance: £%10.%11\n"
    "Runs Completed: %12")
    .arg(m_spinCount)
    .arg(m_money / 100).arg(m_money % 100, 2, 10, QChar('0'))
    .arg(m_maxMoney / 100).arg(m_maxMoney % 100, 2, 10, QChar('0'))
    .arg(m_totalSpins)
    .arg(m_totalMoneyEarnt / 100).arg(m_totalMoneyEarnt % 100, 2, 10, QChar('0'))
    .arg(m_highestSpin)
    .arg(m_allTimeHighestMoney / 100).arg(m_allTimeHighestMoney % 100, 2, 10, QChar('0'))
    .arg(m_runsPlayed)
);
    
    QFont statsFont;
    statsFont.setPointSize(18);
    statsFont.setBold(true);
    statsLabel->setFont(statsFont);
    statsLabel->setAlignment(Qt::AlignCenter);
    statsLabel->setStyleSheet(
        "color: #FFD700;"
        "background-color: rgba(0, 0, 0, 100);"
        "border: 2px solid #FFD700;"
        "border-radius: 15px;"
        "padding: 20px;"
    );
    statsLabel->adjustSize();
    statsLabel->move(
        (m_screenGeometry.width() - statsLabel->width()) / 2,
        m_screenGeometry.height() / 2 - 270
    );

    // Restart button setup
    auto* restartButton = new RotatableButton("Back to Menu", this);
    setupButton(restartButton, getDefaultButtonStyle());
    restartButton->move(
        (m_screenGeometry.width() - BUTTON_WIDTH) / 2,
        m_screenGeometry.height() / 2 + 100
    );
    restartButton->raise();

    // Connect button signals
    connect(restartButton, &QPushButton::pressed, this, &MainWindow::onButtonPressed);
    connect(restartButton, &QPushButton::released, this, &MainWindow::onButtonReleased);
    connect(restartButton, &QPushButton::clicked, this, [this]() {
        // Reset game state
        m_money = 100;
        m_spinCount = 0;
        m_maxMoney = 100;
        clearScreen(0);
    });

    // Exit button setup
    m_exitButton = new RotatableButton("Exit", this);
    setupButton(m_exitButton, getDefaultButtonStyle());
    m_exitButton->move(
        (m_screenGeometry.width() - BUTTON_WIDTH) / 2,
        m_screenGeometry.height() / 2 + 190  // Below restart button
    );

    connect(m_exitButton, &QPushButton::clicked, this, [this]() {
        QApplication::quit();
    });
    connect(m_exitButton, &QPushButton::pressed, this, &MainWindow::onButtonPressed);
    connect(m_exitButton, &QPushButton::released, this, &MainWindow::onButtonReleased);

    // Show all widgets
    backgroundWidget->show();
    frameWidget->show();
    titleLabel->show();
    statsLabel->show();
    restartButton->show();
    m_exitButton->show();
}
