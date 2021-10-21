#include <FAST/Visualization/Window.hpp>
#include <FAST/Pipeline.hpp>
#include <FAST/Tools/CommandLineParser.hpp>
#include <QPushButton>
#include <QVBoxLayout>
#include <FAST/Importers/WholeSlideImageImporter.hpp>
#include <FAST/Data/ImagePyramid.hpp>

namespace fast {

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

    class GUI : public Window {
        FAST_OBJECT(GUI)
    public:
        FAST_CONSTRUCTOR(GUI)
    private:
    };

    static ImagePyramid::pointer loadWSI() {
        auto importer = WholeSlideImageImporter::create(Config::getTestDataPath() + "WSI/A05.svs");
        return importer->runAndGetOutputData<ImagePyramid>();
    }

    GUI::GUI() {
        auto layout = new QVBoxLayout;
        mWidget->setLayout(layout);

        // Load two WSIs
        std::vector<ImagePyramid::pointer> WSIs = {loadWSI(), loadWSI()};

        auto view = createView();
        view->setSynchronizedRendering(false);
        layout->addWidget(view);

        auto button = new QPushButton;
        button->setText("Click me!");
        layout->addWidget(button);
        int counter = 0;
        QObject::connect(button, &QPushButton::clicked, [this, layout, &counter, WSIs]() {
            auto oldView = getView(0);
            mWidget->clearViews();

            auto pipeline = Pipeline(std::string(ROOT_DIR) + "/wsi_patch_segmentation.fpl");
            auto po = EmptyProcessObject::create(WSIs[counter % 2]);
            pipeline.parse({{"WSI", po}});
            ++counter;
            //stopComputationThread();
            for(auto view : pipeline.getViews()) {
                view->setSynchronizedRendering(false);
                layout->replaceWidget(oldView, view);
                mWidget->addView(view);
            }
            // Stop old view and delete it!
            oldView->stopPipeline();
            delete oldView;
            //startComputationThread();
        });
    }
}

using namespace fast;

int main(int argc, char** argv) {
    CommandLineParser parser("");
    parser.parse(argc, argv);
    auto gui = GUI::create();
    gui->run();
}
