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
    private:
        std::vector<ImagePyramid::pointer> m_WSIs;
        int m_currentWSI = 0;
        bool m_procesessing = false;
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

        // Initial empty view
        auto view = createView();
        layout->addWidget(view);

        // Load two WSIs
        m_WSIs = {
                loadWSI(Config::getTestDataPath() + "WSI/A05.svs"),
                loadWSI("/home/smistad/data/FastPathology/WSIs/7.vsi"),
                loadWSI("/home/smistad/data/FastPathology/WSIs/11435_CD3.ndpi")
        };
        for(int i = 0; i < m_WSIs.size(); ++i) {
            auto button = new QPushButton;
            button->setText("Select WSI " + QString::number(i));
            layout->addWidget(button);
            QObject::connect(button, &QPushButton::clicked, std::bind(&GUI::selectWSI, this, i));
        }

        // Load pipelines and create one button for each.
        std::string pipelineFolder = std::string(ROOT_DIR) + "/pipelines/";
        for(auto& filename : getDirectoryList(pipelineFolder)) {
            auto button = new QPushButton;
            auto pipeline = Pipeline(join(pipelineFolder, filename));
            button->setText(("Run pipeline: " + pipeline.getName()).c_str());
            layout->addWidget(button);
            QObject::connect(button, &QPushButton::clicked, std::bind(&GUI::processPipeline, this, join(pipelineFolder, filename)));
        }

        // Stop button
        auto stopButton = new QPushButton;
        stopButton->setText("Stop");
        layout->addWidget(stopButton);
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
        stopProcessing();
        selectWSI(m_currentWSI);
    }

    void GUI::done() {
        if(m_procesessing) {
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
        std::cout << "Loading pipeline.. thread: " << std::this_thread::get_id() << std::endl;
        auto pipeline = Pipeline(pipelinePath);
        std::cout << "OK" << std::endl;
        // parse() only accepts POs for now, so use EmptyProcessObject to give it the WSI
        std::cout << "OK" << std::endl;
        try {
            pipeline.parse({{"WSI", m_WSIs[m_currentWSI]}});
        } catch(Exception &e) {
            m_procesessing = false;
            // Syntax error in pipeline file. Raise error and return to avoid crash.
            QMessageBox msgBox;
            std::string msg = "Error parsing pipeline! " + std::string(e.what());
            msgBox.setText(msg.c_str());
            msgBox.exec();
            return;
        }
        std::cout << "Done" << std::endl;

        for(auto renderer : pipeline.getRenderers()) {
            view->addRenderer(renderer);
        }
        view->reinitialize();
        //compThread->start();
        getComputationThread()->reset();
    }

    void GUI::selectWSI(int i) {
        stopProcessing();
        m_currentWSI = i;
        auto view = getView(0);
        view->removeAllRenderers();
        auto renderer = ImagePyramidRenderer::create()->connect(m_WSIs[m_currentWSI]);
        view->addRenderer(renderer);
        view->reinitialize();
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
