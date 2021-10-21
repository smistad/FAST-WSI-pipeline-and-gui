#include <FAST/Visualization/Window.hpp>
#include <FAST/Pipeline.hpp>
#include <FAST/Tools/CommandLineParser.hpp>
#include <QPushButton>
#include <QVBoxLayout>

namespace fast {

    class GUI : public Window {
        FAST_OBJECT(GUI)
    public:
        FAST_CONSTRUCTOR(GUI)
    private:
    };

    GUI::GUI() {
        auto layout = new QVBoxLayout;
        mWidget->setLayout(layout);

        auto view = createView();
        view->setSynchronizedRendering(false);
        layout->addWidget(view);

        auto button = new QPushButton;
        button->setText("Click me!");
        layout->addWidget(button);
        QObject::connect(button, &QPushButton::clicked, [this, layout]() {
            auto oldView = getView(0);
            mWidget->clearViews();

            auto pipeline = Pipeline("/home/smistad/workspace/fp-test/wsi_patch_segmentation.fpl");
            pipeline.parse();
            //stopComputationThread();
            for(auto view : pipeline.getViews()) {
                view->setSynchronizedRendering(false);
                layout->replaceWidget(oldView, view);
                mWidget->addView(view);
            }
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
