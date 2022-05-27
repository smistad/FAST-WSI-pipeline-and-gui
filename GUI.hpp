#pragma once

#include <FAST/Visualization/Window.hpp>
#include <FAST/Data/ImagePyramid.hpp>

class QProgressDialog;

namespace fast {
    // Make a tiny GUI
    class GUI : public Window {
        FAST_OBJECT(GUI)
		Q_OBJECT
    public:
        FAST_CONSTRUCTOR(GUI);
        void done();
        void stop();
        void stopProcessing();
        void selectWSI(int i);
        void loadResults(int i);
        void processPipeline(std::string pipelinePath);
        void batchProcessPipeline(std::string pipelinePath);
        void saveResults();
        void showMessage(QString msg);
        void runInThread(std::string pipelineFilename, std::string pipelineName, bool runForAll);
	signals:
        void messageSignal(QString msg);
    private:
        std::vector<std::pair<std::string, ImagePyramid::pointer>> m_WSIs;
        int m_currentWSI = 0;
        bool m_procesessing = false;
        bool m_batchProcesessing = false;
        std::unique_ptr<Pipeline> m_runningPipeline;
        QProgressDialog* m_progressDialog;
    };
}

