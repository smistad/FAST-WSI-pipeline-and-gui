#include <FAST/Visualization/Window.hpp>
#include <FAST/Pipeline.hpp>
#include <FAST/Tools/CommandLineParser.hpp>
#include <QPushButton>
#include <QVBoxLayout>
#include <QMessageBox>
#include <FAST/Importers/WholeSlideImageImporter.hpp>
#include <FAST/Visualization/ImagePyramidRenderer/ImagePyramidRenderer.hpp>
#include <FAST/Data/ImagePyramid.hpp>

namespace fast {
    // Make a tiny GUI
    class GUI : public Window {
        FAST_OBJECT(GUI)
    public:
        FAST_CONSTRUCTOR(GUI);
		void done();
		void stop();
		void stopProcessing();
        void selectWSI(int i);
		void processPipeline(std::string pipelinePath);
		void batchProcessPipeline(std::string pipelinePath);
    private:
        std::vector<ImagePyramid::pointer> m_WSIs;
        int m_currentWSI = 0;
        bool m_procesessing = false;
        bool m_batchProcesessing = false;
        std::unique_ptr<Pipeline> m_runningPipeline;
    };

    // Function for loading a test WSI
    static ImagePyramid::pointer loadWSI(std::string filename) {
        auto importer = WholeSlideImageImporter::create(filename);
        return importer->runAndGetOutputData<ImagePyramid>();
    }

    GUI::GUI() {
        // Create GUI
        auto layout = new QVBoxLayout;
        mWidget->setLayout(layout);
        setHeight(getScreenHeight());

        // Create view
        auto view = createView();
        view->setAutoUpdateCamera(true);
        layout->addWidget(view);

        auto bottomLayout = new QHBoxLayout;
        layout->addLayout(bottomLayout);

        auto leftLayout = new QVBoxLayout;
        bottomLayout->addLayout(leftLayout);

        auto rightLayout = new QVBoxLayout;
        bottomLayout->addLayout(rightLayout);

        // Load WSIs from folder
        std::string imageFolder = join(ROOT_DIR, "images");
        for(auto filename : getDirectoryList(imageFolder)) {
            if(filename == ".gitkeep") continue;
            m_WSIs.push_back(loadWSI(join(imageFolder, filename)));
            auto button = new QPushButton;
            button->setText(("Select " + filename).c_str());
            leftLayout->addWidget(button);
            QObject::connect(button, &QPushButton::clicked, std::bind(&GUI::selectWSI, this, m_WSIs.size()-1));
        }

        // Load pipelines and create one button for each.
        std::string pipelineFolder = std::string(ROOT_DIR) + "/pipelines/";
        for(auto& filename : getDirectoryList(pipelineFolder)) {
            auto pipeline = Pipeline(join(pipelineFolder, filename));

            auto button = new QPushButton;
            button->setText(("Run: " + pipeline.getName()).c_str());
            rightLayout->addWidget(button);
            QObject::connect(button, &QPushButton::clicked, std::bind(&GUI::processPipeline, this, join(pipelineFolder, filename)));

            auto batchButton = new QPushButton;
            batchButton->setText(("Run for all: " + pipeline.getName()).c_str());
            rightLayout->addWidget(batchButton);
            QObject::connect(batchButton, &QPushButton::clicked, std::bind(&GUI::batchProcessPipeline, this, join(pipelineFolder, filename)));
        }

        // Stop button
        auto stopButton = new QPushButton;
        stopButton->setText("Stop");
        leftLayout->addWidget(stopButton);
        QObject::connect(stopButton, &QPushButton::clicked, this, &GUI::stop);

        // Notify GUI when pipeline has finished
        QObject::connect(getComputationThread().get(), &ComputationThread::pipelineFinished, this, &GUI::done);
    }

    void GUI::stopProcessing() {
        m_procesessing = false;
        auto view = getView(0);
        std::cout << "stopping pipeline" << std::endl;
        view->stopPipeline();
        std::cout << "done" << std::endl;
        std::cout << "removing renderers.." << std::endl;
        view->removeAllRenderers();
        std::cout << "done" << std::endl;
    }

    void GUI::stop() {
        m_batchProcesessing = false;
        stopProcessing();
        selectWSI(m_currentWSI);
    }

    void GUI::done() {
        if(m_batchProcesessing) {
            if(m_currentWSI == m_WSIs.size()-1) {
                // All processed. Stop.
                m_batchProcesessing = false;
                m_procesessing = false;
                QMessageBox msgBox;
                msgBox.setText("Batch processing is done!");
                msgBox.exec();
            } else {
                // Run next
                m_currentWSI += 1;
                processPipeline(m_runningPipeline->getFilename());
            }
        } else if(m_procesessing) {
            QMessageBox msgBox;
            msgBox.setText("Processing is done!");
            msgBox.exec();
        }
        m_procesessing = false;
        // TODO Save any pipeline output data to disk.
    }

    void GUI::processPipeline(std::string pipelinePath) {
        std::cout << "Processing pipeline: " << pipelinePath << std::endl;
        stopProcessing();
        m_procesessing = true;
        auto view = getView(0);

        // Load pipeline and give it a WSI
        std::cout << "Loading pipeline in thread: " << std::this_thread::get_id() << std::endl;
        m_runningPipeline = std::make_unique<Pipeline>(pipelinePath);
        std::cout << "OK" << std::endl;
        try {
            m_runningPipeline->parse({{"WSI", m_WSIs[m_currentWSI]}});
        } catch(Exception &e) {
            m_procesessing = false;
            m_batchProcesessing = false;
            m_runningPipeline.reset();
            // Syntax error in pipeline file. Raise error and return to avoid crash.
            QMessageBox msgBox;
            std::string msg = "Error parsing pipeline! " + std::string(e.what());
            msgBox.setText(msg.c_str());
            msgBox.exec();
            return;
        }
        std::cout << "Done" << std::endl;

        for(auto renderer : m_runningPipeline->getRenderers()) {
            view->addRenderer(renderer);
        }
        getComputationThread()->reset();
    }

    void GUI::batchProcessPipeline(std::string pipelineFilename) {
        m_currentWSI = 0;
        m_batchProcesessing = true;
        processPipeline(pipelineFilename);
    }

    void GUI::selectWSI(int i) {
        stopProcessing();
        m_currentWSI = i;
        auto view = getView(0);
        view->removeAllRenderers();
        auto renderer = ImagePyramidRenderer::create()->connect(m_WSIs[m_currentWSI]);
        view->addRenderer(renderer);
    }
}

using namespace fast;

int main(int argc, char** argv) {
    // Just start the GUI application
    CommandLineParser parser("");
    parser.parse(argc, argv);
    auto gui = GUI::create();
    gui->run();
}
