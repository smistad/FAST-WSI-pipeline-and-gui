#include <FAST/Visualization/Window.hpp>
#include <FAST/Pipeline.hpp>
#include <FAST/Tools/CommandLineParser.hpp>
#include <QPushButton>
#include <QVBoxLayout>
#include <FAST/Importers/WholeSlideImageImporter.hpp>
#include <FAST/Data/ImagePyramid.hpp>

namespace fast {

    // Used as a trick since Pipeline::parse doesn't accept data objects, just process objects for now..
    class EmptyProcessObject : public ProcessObject {
        FAST_OBJECT_V4(EmptyProcessObject)
        public:
            FAST_CONSTRUCTOR(EmptyProcessObject, DataObject::pointer, data,)
        private:
            void execute() {
                addOutputData(0, mData);
            };
            DataObject::pointer mData;
    };

    EmptyProcessObject::EmptyProcessObject(DataObject::pointer data) {
        createOutputPort<DataObject>(0);
        mData = data;
        mIsModified = true;
    };

    // Make a tiny GUI
    class GUI : public Window {
        FAST_OBJECT(GUI)
    public:
        FAST_CONSTRUCTOR(GUI)
    private:
    };

    // Function for loading a test WSI
    static ImagePyramid::pointer loadWSI() {
        auto importer = WholeSlideImageImporter::create(Config::getTestDataPath() + "WSI/A05.svs");
        return importer->runAndGetOutputData<ImagePyramid>();
    }

    GUI::GUI() {
        // Create GUI
        auto layout = new QVBoxLayout;
        mWidget->setLayout(layout);

        // Initial empty view
        auto view = createView();
        view->setSynchronizedRendering(false);
        layout->addWidget(view);

        // Load two WSIs
        std::vector<ImagePyramid::pointer> WSIs = {loadWSI(), loadWSI()};

        // A button to trigger event
        auto button = new QPushButton;
        button->setText("Click me!");
        layout->addWidget(button);

        int counter = 0; // Used to switch between the two WSIs

        // Every time the button is clicked:
        QObject::connect(button, &QPushButton::clicked, [this, layout, &counter, WSIs]() {
            // Get old view, and remove it from Widget
            auto oldView = getView(0);
            oldView->stopPipeline();
            oldView->removeAllRenderers();

            // Load pipeline and give it a WSI
            std::cout << "Loading pipeline.. thread:" << std::this_thread::get_id() << std::endl;
            auto pipeline = Pipeline(std::string(ROOT_DIR) + "/wsi_patch_segmentation.fpl");
            // parse() only accepts POs for now, so use EmptyProcessObject to give it the WSI
            auto po = EmptyProcessObject::create(WSIs[counter % 2]);
            pipeline.parse({{"WSI", po}});
            std::cout << "Done" << std::endl;
            ++counter;
            // Get all renderers
            for(auto renderer : pipeline.getRenderers()) {
                renderer->setSynchronizedRendering(false); // Disable synchronized rendering
            }
            for(auto renderer : pipeline.getRenderers()) {
                oldView->addRenderer(renderer);
            }
            oldView->reinitialize();
        });
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
