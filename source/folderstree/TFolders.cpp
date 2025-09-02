#include "SPEditor.h"
#include "SPFolders.h"

#include <QVBoxLayout>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QHeaderView>
#include <QPainter>

FolderTree::FolderTree(SPEditor* textEditor, QWidget* parent)
    : QDockWidget(parent), textEditor(textEditor)
{
    setWindowTitle("الملفات");
    setFont(QFont("Tajawal"));
    setStyleSheet(R"(
        QDockWidget {
            color: #dddddd;
            border: none;
            titlebar-close-icon: url(:/Resources/close.png);
        }
        QDockWidget::title {
            background-color: #1e202e;
            border: none;
            padding: 3px 5px 0 0;
        }
        QDockWidget::close-button {
            icon-size: 10px;
        }
    )");

    setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable);

    // Create tree view
    treeView = new QTreeView(this);
    treeView->setStyleSheet(R"(
        QTreeView {
            color: white;
            background-color: #141520;
            border: none;
        }
    )");

    // Hide the header (title bar)
    treeView->header()->hide();

    // Create file system model
    fileModel = new QFileSystemModel(this);

    // Set root to the directory of the current application
    QString projectPath = QDir::currentPath() + "/..";
    fileModel->setRootPath(projectPath);

    // Set the model for the tree view
    treeView->setModel(fileModel);
    treeView->setRootIndex(fileModel->index(projectPath));

    // Hide unnecessary columns
    for (int i = 1; i < fileModel->columnCount(); ++i) {
        treeView->hideColumn(i);
    }

    // Set up layout
    QWidget* containerWidget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(containerWidget);
    layout->addWidget(treeView);
    layout->setContentsMargins(0, 0, 0, 0);

    // Set the container widget as the dock widget's widget
    setWidget(containerWidget);

    // Allow docking on left and right sides
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    // Setup connections
    setupConnections();
}

void FolderTree::setupConnections()
{
    // Connect double-click signal to handle file opening
    connect(treeView, &QTreeView::doubleClicked, this, &FolderTree::onFileDoubleClicked);
}

void FolderTree::onFileDoubleClicked(const QModelIndex& index)
{
    QString filePath = fileModel->filePath(index);
    QFileInfo fileInfo(filePath);

    // Check if it's a file (not a directory)
    if (fileInfo.isFile()) {
        // List of text file extensions
        QStringList textExtensions = { "alif", "txt", "log", "md", "csv",
            "json", "xml", "html", "css", "js", "cpp", "h", "py" };

        QString ext = fileInfo.suffix().toLower();
        if (textExtensions.contains(ext)) {
            // Open text file
            QFile file(filePath);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&file);
                QString content = in.readAll();
                file.close();

                // Set content in text editor
                if (textEditor) {
                    textEditor->setPlainText(content);
                }
            }
            else {
                // Error opening file
                QMessageBox::warning(this, "Error",
                    QString("Could not open file: %1\n%2")
                    .arg(filePath, file.errorString())
                );
            }
        }
    }
}

void FolderTree::setRootPath(const QString& path)
{
    // Set the root path for the file system model
    treeView->setRootIndex(fileModel->index(path));
}





void FolderTree::openFolder()
{
    // Open folder selection dialog
    QString selectedFolder = QFileDialog::getExistingDirectory(
        this,
        tr("اختر ملف"),
        projectPath,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    // Check if a folder was selected
    if (!selectedFolder.isEmpty()) {
        // Update project path
        projectPath = selectedFolder;

        // Set the root index of the tree view to the selected folder
        QModelIndex rootIndex = fileModel->setRootPath(selectedFolder);
        treeView->setRootIndex(rootIndex);

        // Optional: Expand the root index
        treeView->expand(rootIndex);

        // Emit signal that folder has changed
        emit folderChanged(selectedFolder);

        // Optional: Log the folder change
        qDebug() << "Folder changed to: " << selectedFolder;
    }
}