/**********************************************************************
*  Copyright (c) 2008-2015, Alliance for Sustainable Energy.  
*  All rights reserved.
*  
*  This library is free software; you can redistribute it and/or
*  modify it under the terms of the GNU Lesser General Public
*  License as published by the Free Software Foundation; either
*  version 2.1 of the License, or (at your option) any later version.
*  
*  This library is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*  Lesser General Public License for more details.
*  
*  You should have received a copy of the GNU Lesser General Public
*  License along with this library; if not, write to the Free Software
*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**********************************************************************/
#include "OSServer.hpp"
#include "OSServer_Impl.hpp"

#include "../core/Application.hpp"
#include "../core/System.hpp"
#include "../core/Json.hpp"
#include "../core/Assert.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QHttpMultiPart>
#include <QMutex>
#include <QFile>

namespace openstudio{
  namespace detail{

    OSServer_Impl::OSServer_Impl(const QUrl& url)
      : QObject(), m_url(url), m_networkAccessManager(new QNetworkAccessManager()),
        m_networkReply(nullptr), m_mutex(new QMutex()),
        m_lastAvailable(false),
        m_lastProjectUUIDs(), 
        m_lastCreateProjectSuccess(false),
        m_lastDeleteProjectSuccess(false),
        m_lastAnalysisUUIDs(),
        m_lastPostAnalysisJSONSuccess(false),
        m_lastPostDataPointJSONSuccess(false),
        m_lastUploadAnalysisFilesSuccess(false),
        m_lastStartSuccess(false),
        m_lastIsAnalysisQueued(false),
        m_lastIsAnalysisRunning(false),
        m_lastIsAnalysisComplete(false),
        m_lastStopSuccess(false),
        m_lastDataPointUUIDs(),
        m_lastQueuedDataPointUUIDs(),
        m_lastRunningDataPointUUIDs(),
        m_lastCompleteDataPointUUIDs(),
        m_lastDataPointJSON(),
        m_lastDownloadDataPointSuccess(false),
        m_lastDeleteDataPointSuccess(false),
        m_errors(),
        m_warnings()
    {
      //Make sure a QApplication exists
      openstudio::Application::instance().application(false);

      if (m_url.scheme().isEmpty()){
        QString urlString = m_url.toString();
        //LOG(Debug, "Url before: " << toString(urlString));
        //LOG(Debug, "Url valid: " << m_url.isValid());
        //LOG(Debug, "Url relative: " << m_url.isRelative());
        
        //m_url.setScheme("http"); // DLM: this was not adding //
        if (urlString.startsWith("//")){
          m_url.setUrl("http:" + urlString);
        }else{
          m_url.setUrl("http://" + urlString);
        }

        //LOG(Debug, "Url after: " << toString(m_url.toString()));
        //LOG(Debug, "Url valid: " << m_url.isValid());
        //LOG(Debug, "Url relative: " << m_url.isRelative());
      }
    }

    OSServer_Impl::~OSServer_Impl()
    {
      resetNetworkReply();
    }
 
    bool OSServer_Impl::available(int msec)
    {
      if (requestAvailable()){
        if (waitForFinished(msec)){
          return lastAvailable();
        }
      }
      return false;
    }

    bool OSServer_Impl::lastAvailable() const
    {
      return m_lastAvailable;
    }

    std::vector<UUID> OSServer_Impl::projectUUIDs(int msec)
    {
      if (requestProjectUUIDs()){
        if (waitForFinished(msec)){
          return lastProjectUUIDs();
        }
      }
      return std::vector<UUID>();
    }

    std::vector<UUID> OSServer_Impl::lastProjectUUIDs() const
    {
      return m_lastProjectUUIDs;
    }

    bool OSServer_Impl::createProject(const UUID& projectUUID, int msec) 
    {
      if (requestCreateProject(projectUUID)){
        if (waitForFinished(msec)){
          return lastCreateProjectSuccess();
        }
      }
      return false;
    }

    bool OSServer_Impl::lastCreateProjectSuccess() const
    {
      return m_lastCreateProjectSuccess;
    }

    bool OSServer_Impl::deleteProject(const UUID& projectUUID, int msec) 
    {
      if (requestDeleteProject(projectUUID)){
        if (waitForFinished(msec)){
          return lastDeleteProjectSuccess();
        }
      }
      return false;
    }

    bool OSServer_Impl::lastDeleteProjectSuccess() const
    {
      return m_lastDeleteProjectSuccess;
    }

    std::vector<UUID> OSServer_Impl::analysisUUIDs(const UUID& projectUUID, int msec)
    {
      if (requestAnalysisUUIDs(projectUUID)){
        if (waitForFinished(msec)){
          return lastAnalysisUUIDs();
        }
      }
      return std::vector<UUID>();
    }

    std::vector<UUID> OSServer_Impl::lastAnalysisUUIDs() const
    {
      return m_lastAnalysisUUIDs;
    } 

    bool OSServer_Impl::postAnalysisJSON(const UUID& projectUUID, const std::string& analysisJSON, int msec)
    {
      if (startPostAnalysisJSON(projectUUID, analysisJSON)){
        if (waitForFinished(msec)){
          return lastPostAnalysisJSONSuccess();
        }
      }
      return false;
    }

    bool OSServer_Impl::lastPostAnalysisJSONSuccess() const
    {
      return m_lastPostAnalysisJSONSuccess;
    }

    bool OSServer_Impl::postDataPointJSON(const UUID& analysisUUID, const std::string& dataPointJSON, int msec)
    {
      if (startPostDataPointJSON(analysisUUID, dataPointJSON)){
        if (waitForFinished(msec)){
          return lastPostDataPointJSONSuccess();
        }
      }
      return false;
    }

    bool OSServer_Impl::lastPostDataPointJSONSuccess() const
    {
      return m_lastPostDataPointJSONSuccess;
    }

    bool OSServer_Impl::uploadAnalysisFiles(const UUID& analysisUUID, const openstudio::path& analysisZipFile, int msec)
    {
      if (startUploadAnalysisFiles(analysisUUID, analysisZipFile)){
        if (waitForFinished(msec)){
          return lastUploadAnalysisFilesSuccess();
        }
      }
      return false;
    }

    bool OSServer_Impl::lastUploadAnalysisFilesSuccess() const
    {
      return m_lastUploadAnalysisFilesSuccess;
    }

    bool OSServer_Impl::start(const UUID& analysisUUID, int msec)
    {
      if (requestStart(analysisUUID)){
        if (waitForFinished(msec)){
          return lastStartSuccess();
        }
      }
      return false;
    }

    bool OSServer_Impl::lastStartSuccess() const
    {
      return m_lastStartSuccess;
    }

    bool OSServer_Impl::isAnalysisQueued(const UUID& analysisUUID, int msec)
    {
      if (requestIsAnalysisQueued(analysisUUID)){
        if (waitForFinished(msec)){
          return lastIsAnalysisQueued();
        }
      }
      return false;
    }

    bool OSServer_Impl::lastIsAnalysisQueued() const
    {
      return m_lastIsAnalysisQueued;
    }

    bool OSServer_Impl::isAnalysisRunning(const UUID& analysisUUID, int msec)
    {
      if (requestIsAnalysisRunning(analysisUUID)){
        if (waitForFinished(msec)){
          return lastIsAnalysisRunning();
        }
      }
      return false;
    }

    bool OSServer_Impl::lastIsAnalysisRunning() const
    {
      return m_lastIsAnalysisRunning;
    }

    bool OSServer_Impl::isAnalysisComplete(const UUID& analysisUUID, int msec)
    {
      if (requestIsAnalysisComplete(analysisUUID)){
        if (waitForFinished(msec)){
          return lastIsAnalysisComplete();
        }
      }
      return false;
    }

    bool OSServer_Impl::lastIsAnalysisComplete() const
    {
      return m_lastIsAnalysisComplete;
    }

    bool OSServer_Impl::stop(const UUID& analysisUUID, int msec)
    {
      if (requestStop(analysisUUID)){
        if (waitForFinished(msec)){
          return lastStopSuccess();
        }
      }
      return false;
    }

    bool OSServer_Impl::lastStopSuccess() const
    {
      return m_lastStopSuccess;
    }

    std::vector<UUID> OSServer_Impl::dataPointUUIDs(const UUID& analysisUUID, int msec)
    {
      if (requestDataPointUUIDs(analysisUUID)){
        if (waitForFinished(msec)){
          return lastDataPointUUIDs();
        }
      }
      return std::vector<UUID>();
    }

    std::vector<UUID> OSServer_Impl::lastDataPointUUIDs() const
    {
      return m_lastDataPointUUIDs;
    }

    std::vector<UUID> OSServer_Impl::queuedDataPointUUIDs(const UUID& analysisUUID, int msec)
    {
      if (requestQueuedDataPointUUIDs(analysisUUID)){
        if (waitForFinished(msec)){
          return lastQueuedDataPointUUIDs();
        }
      }
      return std::vector<UUID>();
    }

    std::vector<UUID> OSServer_Impl::lastQueuedDataPointUUIDs() const
    {
      return m_lastQueuedDataPointUUIDs;
    }

    std::vector<UUID> OSServer_Impl::runningDataPointUUIDs(const UUID& analysisUUID, int msec)
    {
      if (requestRunningDataPointUUIDs(analysisUUID)){
        if (waitForFinished(msec)){
          return lastRunningDataPointUUIDs();
        }
      }
      return std::vector<UUID>();
    }

    std::vector<UUID> OSServer_Impl::lastRunningDataPointUUIDs() const
    {
      return m_lastRunningDataPointUUIDs;
    }

    std::vector<UUID> OSServer_Impl::completeDataPointUUIDs(const UUID& analysisUUID, int msec)
    {
      if (requestCompleteDataPointUUIDs(analysisUUID)){
        if (waitForFinished(msec)){
          return lastCompleteDataPointUUIDs();
        }
      }
      return std::vector<UUID>();
    }

    std::vector<UUID> OSServer_Impl::lastCompleteDataPointUUIDs() const
    {
      return m_lastCompleteDataPointUUIDs;
    }

    std::vector<UUID> OSServer_Impl::downloadReadyDataPointUUIDs(const UUID& analysisUUID, int msec)
    {
      if (requestDownloadReadyDataPointUUIDs(analysisUUID)){
        if (waitForFinished(msec)){
          return lastDownloadReadyDataPointUUIDs();
        }
      }
      return std::vector<UUID>();
    }

    std::vector<UUID> OSServer_Impl::lastDownloadReadyDataPointUUIDs() const
    {
      return m_lastDownloadReadyDataPointUUIDs;
    }

    std::string OSServer_Impl::dataPointJSON(const UUID& analysisUUID, const UUID& dataPointUUID, int msec)
    {
      if (requestDataPointJSON(analysisUUID, dataPointUUID)){
        if (waitForFinished(msec)){
          return lastDataPointJSON();
        }
      }
      return "";
    }

    std::string OSServer_Impl::lastDataPointJSON() const
    {
      return m_lastDataPointJSON;
    }

    bool OSServer_Impl::downloadDataPoint(const UUID& analysisUUID, const UUID& dataPointUUID, const openstudio::path& downloadPath, int msec)
    {
      if (startDownloadDataPoint(analysisUUID, dataPointUUID, downloadPath)){
        if (waitForFinished(msec)){
          return lastDownloadDataPointSuccess();
        }
      }
      return false;
    }

    bool OSServer_Impl::lastDownloadDataPointSuccess() const
    {
      return m_lastDownloadDataPointSuccess;
    }

    bool OSServer_Impl::deleteDataPoint(const UUID& analysisUUID,
                                        const UUID& dataPointUUID,
                                        int msec)
    {
      if (requestDeleteDataPoint(analysisUUID,dataPointUUID)) {
        if (waitForFinished(msec)) {
          return lastDeleteDataPointSuccess();
        }
      }
      return false;
    }

    bool OSServer_Impl::lastDeleteDataPointSuccess() const {
      return m_lastDeleteDataPointSuccess;
    }

    bool OSServer_Impl::waitForFinished(int msec)
    {
      int msecPerLoop = 20;
      int numTries = msec / msecPerLoop;
      int current = 0;
      while (true)
      {
    
        // if we can get the lock then the download is complete
        if (m_mutex->tryLock()){
          m_mutex->unlock();
          return true;
        }

        // this calls process events
        System::msleep(msecPerLoop);

        if (current > numTries){
          logError("Response timed out");
          break;
        }

        ++current;
      }

      QObject::disconnect(m_networkReply, nullptr, this, nullptr);
      resetNetworkReply();
      m_mutex->unlock();

      emit requestProcessed(false);

      return false;
    }

    std::vector<std::string> OSServer_Impl::errors() const
    {
      return m_errors;
    }

    std::vector<std::string> OSServer_Impl::warnings() const
    {
      return m_warnings;
    }

    bool OSServer_Impl::requestAvailable()
    {

      clearErrorsAndWarnings();

      m_lastAvailable = false;

      if (!m_mutex->tryLock()){
        return false;
      }

      QUrl url(m_url);
      QNetworkRequest request(url);
      m_networkReply = m_networkAccessManager->get(request);

      connect(m_networkReply, &QNetworkReply::finished, this, &OSServer_Impl::processAvailable);

      return true;
    }

    bool OSServer_Impl::requestProjectUUIDs()
    {

      clearErrorsAndWarnings();

      m_lastProjectUUIDs.clear();

      if (!m_mutex->tryLock()){
        return false;
      }

      QUrl url(m_url.toString().append("/projects.json"));
      QNetworkRequest request(url);
      m_networkReply = m_networkAccessManager->get(request);

      connect(m_networkReply, &QNetworkReply::finished, this, &OSServer_Impl::processProjectUUIDs);

      return true;
    } 
    
    bool OSServer_Impl::requestCreateProject(const UUID& projectUUID)
    {

      clearErrorsAndWarnings();

      m_lastCreateProjectSuccess = false;

      if (!m_mutex->tryLock()){
        return false;
      }

      QString id = toQString(removeBraces(projectUUID));
      QUrl url(m_url.toString().append("/projects.json"));
  
      QJsonObject obj;
      obj["_id"] = QJsonValue(id);
      obj["name"] = QJsonValue(id);
      obj["uuid"] = QJsonValue(id);
      obj["analyses"] = QJsonValue(QJsonArray());
      
      QByteArray json = QJsonDocument(obj).toJson(QJsonDocument::Compact);

      QNetworkRequest request(url);
      request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

      m_networkReply = m_networkAccessManager->post(request, json);

      connect(m_networkReply, &QNetworkReply::finished, this, &OSServer_Impl::processCreateProject);

      return true;
    }

    bool OSServer_Impl::requestDeleteProject(const UUID& projectUUID)
    {

      clearErrorsAndWarnings();

      m_lastDeleteProjectSuccess = false;

      if (!m_mutex->tryLock()){
        return false;
      }

      QString id = toQString(removeBraces(projectUUID));
      QUrl url(m_url.toString().append("/projects/").append(id));
      QNetworkRequest request(url);
      m_networkReply = m_networkAccessManager->deleteResource(request);

      connect(m_networkReply, &QNetworkReply::finished, this, &OSServer_Impl::processDeleteProject);

      return true;
    } 

    bool OSServer_Impl::requestAnalysisUUIDs(const UUID& projectUUID)
    {

      clearErrorsAndWarnings();

      m_lastAnalysisUUIDs.clear();

      if (!m_mutex->tryLock()){
        return false;
      }

      QString id = toQString(removeBraces(projectUUID));
      QUrl url(m_url.toString().append("/projects/").append(id).append(".json"));
      QNetworkRequest request(url);
      m_networkReply = m_networkAccessManager->get(request);

      connect(m_networkReply, &QNetworkReply::finished, this, &OSServer_Impl::processAnalysisUUIDs);
      
      return true;
    } 

    bool OSServer_Impl::startPostAnalysisJSON(const UUID& projectUUID, const std::string& analysisJSON)
    {

      clearErrorsAndWarnings();

      m_lastPostAnalysisJSONSuccess = false;

      if (!m_mutex->tryLock()){
        return false;
      }

      QString id = toQString(removeBraces(projectUUID));
      QUrl url(m_url.toString().append("/projects/").append(id).append("/analyses.json"));

      QByteArray postData; 
      postData.append(toQString(analysisJSON)); 

      QNetworkRequest request(url);
      request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

      m_networkReply = m_networkAccessManager->post(request, postData);

      connect(m_networkReply, &QNetworkReply::finished, this, &OSServer_Impl::processPostAnalysisJSON);
      
      connect(m_networkReply, &QNetworkReply::uploadProgress, this, &OSServer_Impl::logUploadProgress);

      return true;
    }

    bool OSServer_Impl::startPostDataPointJSON(const UUID& analysisUUID, const std::string& dataPointJSON)
    {

      clearErrorsAndWarnings();

      m_lastPostDataPointJSONSuccess = false;

      if (!m_mutex->tryLock()){
        return false;
      }

      QString id = toQString(removeBraces(analysisUUID));
      // ETH: We are only using batch_upload now. Should rails application also just make 
      // data_points.json be a batch upload. (Deprecate old, single point functionality, and move
      // data_points/batch_upload.json to be data_points.json?)
      QUrl url(m_url.toString().append("/analyses/").append(id).append("/data_points/batch_upload.json"));

      QByteArray postData; 
      postData.append(toQString(dataPointJSON)); 

      QNetworkRequest request(url);
      request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

      m_networkReply = m_networkAccessManager->post(request, postData);

      connect(m_networkReply, &QNetworkReply::finished, this, &OSServer_Impl::processPostDataPointJSON);
      
      connect(m_networkReply, &QNetworkReply::uploadProgress, this, &OSServer_Impl::logUploadProgress);
      
      return true;
    }

    bool OSServer_Impl::startUploadAnalysisFiles(const UUID& analysisUUID, const openstudio::path& analysisZipFile)
    {

      clearErrorsAndWarnings();

      m_lastUploadAnalysisFilesSuccess = false;

      if (!m_mutex->tryLock()){
        return false;
      }

      if (exists(analysisZipFile)){

        QFile file(toQString(analysisZipFile));
        if (file.open(QIODevice::ReadOnly)){

          QString bound="-------3dpj1k39xoa84u4804ee1156snfxl6"; 

          QByteArray data(QString("--" + bound + "\r\n").toLatin1());
          data += "Content-Disposition: form-data; name=\"file\"; filename=\"project.zip\"\r\n";
          data += "Content-Type: application/zip\r\n\r\n";
          data.append(file.readAll());
          data += "\r\n";
          data += QString("--" + bound + "--\r\n.").toLatin1();
          data += "\r\n";
          file.close();

          QString id = toQString(removeBraces(analysisUUID));
          QUrl url(m_url.toString().append("/analyses/").append(id).append("/upload.json")); 

          QNetworkRequest request(url);
          request.setRawHeader(QString("Cache-Control").toLatin1(),QString("no-cache").toLatin1());
          request.setRawHeader(QString("Content-Type").toLatin1(),QString("multipart/form-data; boundary=" + bound).toLatin1());
          request.setRawHeader(QString("Content-Length").toLatin1(), QString::number(data.length()).toLatin1());

          m_networkReply = m_networkAccessManager->post(request, data);

          connect(m_networkReply, &QNetworkReply::finished, this, &OSServer_Impl::processUploadAnalysisFiles);

          connect(m_networkReply, &QNetworkReply::uploadProgress, this, &OSServer_Impl::logUploadProgress);
          
          return true;
        }

      }else{
        logError("File does not exist");
      }

      return false;
    }

    bool OSServer_Impl::requestStart(const UUID& analysisUUID)
    {

      clearErrorsAndWarnings();

      m_lastStartSuccess = false;

      if (!m_mutex->tryLock()){
        return false;
      }

      QString id = toQString(removeBraces(analysisUUID));
      QUrl url(m_url.toString().append("/analyses/").append(id).append("/action.json"));

      QJsonObject obj;
      obj["analysis_action"] = QJsonValue(QString("start"));
      obj["run_data_point_filename"] = QJsonValue(QString("pat_workflow"));
      
      QByteArray json = QJsonDocument(obj).toJson(QJsonDocument::Compact);

      QNetworkRequest request(url);
      request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

      m_networkReply = m_networkAccessManager->post(request, json);

      connect(m_networkReply, &QNetworkReply::finished, this, &OSServer_Impl::processStart);
      
      return true;
    }

    bool OSServer_Impl::requestIsAnalysisQueued(const UUID& analysisUUID)
    {

      clearErrorsAndWarnings();

      m_lastIsAnalysisQueued = false;

      if (!m_mutex->tryLock()){
        return false;
      }

      QString id = toQString(removeBraces(analysisUUID));
      QUrl url(m_url.toString().append("/analyses/").append(id).append("/status.json"));
      QNetworkRequest request(url);
      m_networkReply = m_networkAccessManager->get(request);

      connect(m_networkReply, &QNetworkReply::finished, this, &OSServer_Impl::processIsAnalysisQueued);
      
      return true;
    }

    bool OSServer_Impl::requestIsAnalysisRunning(const UUID& analysisUUID)
    {

      clearErrorsAndWarnings();

      m_lastIsAnalysisRunning = false;

      if (!m_mutex->tryLock()){
        return false;
      }

      QString id = toQString(removeBraces(analysisUUID));
      QUrl url(m_url.toString().append("/analyses/").append(id).append("/status.json"));
      QNetworkRequest request(url);
      m_networkReply = m_networkAccessManager->get(request);

      connect(m_networkReply, &QNetworkReply::finished, this, &OSServer_Impl::processIsAnalysisRunning);

      return true;
    }

    bool OSServer_Impl::requestIsAnalysisComplete(const UUID& analysisUUID)
    {

      clearErrorsAndWarnings();

      m_lastIsAnalysisComplete = false;

      if (!m_mutex->tryLock()){
        return false;
      }

      QString id = toQString(removeBraces(analysisUUID));
      QUrl url(m_url.toString().append("/analyses/").append(id).append("/status.json"));
      QNetworkRequest request(url);
      m_networkReply = m_networkAccessManager->get(request);

      connect(m_networkReply, &QNetworkReply::finished, this, &OSServer_Impl::processIsAnalysisComplete);

      return true;
    }

    bool OSServer_Impl::requestStop(const UUID& analysisUUID)
    {

      clearErrorsAndWarnings();

      m_lastStopSuccess = false;

      if (!m_mutex->tryLock()){
        return false;
      }

      QString id = toQString(removeBraces(analysisUUID));
      QUrl url(m_url.toString().append("/analyses/").append(id).append("/action.json"));

      QJsonObject obj;
      obj["analysis_action"] = QJsonValue(QString("stop"));
      
      QByteArray json = QJsonDocument(obj).toJson(QJsonDocument::Compact);

      QNetworkRequest request(url);
      request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

      m_networkReply = m_networkAccessManager->post(request, json);

      connect(m_networkReply, &QNetworkReply::finished, this, &OSServer_Impl::processStop);

      return true;
    }

    bool OSServer_Impl::requestDataPointUUIDs(const UUID& analysisUUID)
    {

      clearErrorsAndWarnings();

      m_lastDataPointUUIDs.clear();

      if (!m_mutex->tryLock()){
        return false;
      }

      QString id = toQString(removeBraces(analysisUUID));
      QUrl url(m_url.toString().append("/analyses/").append(id).append("/status.json"));
      QNetworkRequest request(url);
      m_networkReply = m_networkAccessManager->get(request);

      connect(m_networkReply, &QNetworkReply::finished, this, &OSServer_Impl::processDataPointUUIDs);
      
      return true;
    }

    bool OSServer_Impl::requestQueuedDataPointUUIDs(const UUID& analysisUUID)
    {
      clearErrorsAndWarnings();

      m_lastQueuedDataPointUUIDs.clear();

      if (!m_mutex->tryLock()){
        return false;
      }

      QString id = toQString(removeBraces(analysisUUID));
      QUrl url(m_url.toString().append("/analyses/").append(id).append("/status.json?jobs=queued"));
      QNetworkRequest request(url);
      m_networkReply = m_networkAccessManager->get(request);

      connect(m_networkReply, &QNetworkReply::finished, this, &OSServer_Impl::processQueuedDataPointUUIDs);
      
      return true;
    }

    bool OSServer_Impl::requestRunningDataPointUUIDs(const UUID& analysisUUID)
    {

      clearErrorsAndWarnings();

      m_lastRunningDataPointUUIDs.clear();

      if (!m_mutex->tryLock()){
        return false;
      }

      QString id = toQString(removeBraces(analysisUUID));
      QUrl url(m_url.toString().append("/analyses/").append(id).append("/status.json?jobs=started"));
      QNetworkRequest request(url);
      m_networkReply = m_networkAccessManager->get(request);

      connect(m_networkReply, &QNetworkReply::finished, this, &OSServer_Impl::processRunningDataPointUUIDs);

      return true;
    }

    bool OSServer_Impl::requestCompleteDataPointUUIDs(const UUID& analysisUUID)
    {
      clearErrorsAndWarnings();

      m_lastCompleteDataPointUUIDs.clear();

      if (!m_mutex->tryLock()){
        return false;
      }

      QString id = toQString(removeBraces(analysisUUID));
      QUrl url(m_url.toString().append("/analyses/").append(id).append("/status.json?jobs=completed"));
      QNetworkRequest request(url);
      m_networkReply = m_networkAccessManager->get(request);

      connect(m_networkReply, &QNetworkReply::finished, this, &OSServer_Impl::processCompleteDataPointUUIDs);

      return true;
    }

    bool OSServer_Impl::requestDownloadReadyDataPointUUIDs(const UUID& analysisUUID)
    {
      clearErrorsAndWarnings();

      m_lastDownloadReadyDataPointUUIDs.clear();

      if (!m_mutex->tryLock()){
        return false;
      }
   
      QString id = toQString(removeBraces(analysisUUID));
      QUrl url(m_url.toString().append("/analyses/").append(id).append("/download_status.json?downloads=completed"));
      QNetworkRequest request(url);
      m_networkReply = m_networkAccessManager->get(request);

      connect(m_networkReply, &QNetworkReply::finished, this, &OSServer_Impl::processDownloadReadyDataPointUUIDs);
      
      return true;
    }

    bool OSServer_Impl::requestDataPointJSON(const UUID& analysisUUID, const UUID& dataPointUUID)
    {
      clearErrorsAndWarnings();

      m_lastDataPointJSON = "";

      if (!m_mutex->tryLock()){
        return false;
      }

      QString id = toQString(removeBraces(dataPointUUID));
      QUrl url(m_url.toString().append("/data_points/").append(id).append(".json"));
      QNetworkRequest request(url);
      m_networkReply = m_networkAccessManager->get(request);

      connect(m_networkReply, &QNetworkReply::finished, this, &OSServer_Impl::processDataPointJSON);
      
      return true;
    }

    bool OSServer_Impl::startDownloadDataPoint(const UUID& analysisUUID, const UUID& dataPointUUID, const openstudio::path& downloadPath)
    {
      clearErrorsAndWarnings();

      m_lastDownloadDataPointSuccess = false;

      m_lastDownloadDataPointPath = downloadPath;

      if (!m_mutex->tryLock()){
        return false;
      }

      QString id = toQString(removeBraces(dataPointUUID));
      QUrl url(m_url.toString().append("/data_points/").append(id).append("/download"));
      QNetworkRequest request(url);
      m_networkReply = m_networkAccessManager->get(request);

      connect(m_networkReply, &QNetworkReply::finished, this, &OSServer_Impl::processDownloadDataPointComplete);
      
      return true;
    }

    bool OSServer_Impl::requestDeleteDataPoint(const UUID& analysisUUID, const UUID& dataPointUUID)
    {
      clearErrorsAndWarnings();

      m_lastDeleteDataPointSuccess = false;

      if (!m_mutex->tryLock()) {
        return false;
      }

      QString id = toQString(removeBraces(dataPointUUID));
      QUrl url(m_url.toString().append("/data_points/").append(id));
      QNetworkRequest request(url);
      m_networkReply = m_networkAccessManager->deleteResource(request);

      connect(m_networkReply, &QNetworkReply::finished, this, &OSServer_Impl::processDeleteDataPoint);
      
      return true;
    }

    void OSServer_Impl::processAvailable()
    {
      bool success = false;

      logNetworkReply("processAvailable");

      if (m_networkReply->error() == QNetworkReply::NoError){
        m_lastAvailable = true;
        success = true;
      }else{
        logNetworkError(m_networkReply->error());
      }

      resetNetworkReply();
      m_mutex->unlock();

      emit requestProcessed(success);
    }

    void OSServer_Impl::processProjectUUIDs()
    {
      bool success = false;

      logNetworkReply("processProjectUUIDs");

      if (m_networkReply->error() == QNetworkReply::NoError){
        QJsonParseError err;
        QJsonDocument json = QJsonDocument::fromJson(m_networkReply->readAll(), &err);
        if (!err.error) {
          m_lastProjectUUIDs = processListOfUUID(json.array(), success);
        }else{
          logError("Incorrect JSON response");
        }
      }else{
        logNetworkError(m_networkReply->error());
      }

      resetNetworkReply();
      m_mutex->unlock();

      emit requestProcessed(success);
    }

    void OSServer_Impl::processCreateProject()
    {
      bool success = false;

      logNetworkReply("processCreateProject");

      if (m_networkReply->error() == QNetworkReply::NoError){
        m_lastCreateProjectSuccess = true;
        success = true;
      }else{
        logNetworkError(m_networkReply->error());
      }

      resetNetworkReply();
      m_mutex->unlock();

      emit requestProcessed(success);     
    }

    void OSServer_Impl::processDeleteProject()
    {
      bool success = false;

      logNetworkReply("processDeleteProject");

      if (m_networkReply->error() == QNetworkReply::NoError){
        m_lastDeleteProjectSuccess = true;
        success = true;
      }else{
        logNetworkError(m_networkReply->error());
      }

      resetNetworkReply();
      m_mutex->unlock();

      emit requestProcessed(success);
    }

    void OSServer_Impl::processAnalysisUUIDs()
    {
      bool success = false;

      logNetworkReply("processAnalysisUUIDs");

      if (m_networkReply->error() == QNetworkReply::NoError){

        QJsonParseError err;
        QJsonDocument json = QJsonDocument::fromJson(m_networkReply->readAll(), &err);
        if (!err.error) {

          if (json.object().contains("analyses")){

            QJsonArray analyses = json.object()["analyses"].toArray();

            m_lastAnalysisUUIDs = processListOfUUID(analyses, success);

          }else{
            logError("Incorrect JSON response");
          }

        }else{
          logError("Could not parse JSON response");
        }

      }else{
        logNetworkError(m_networkReply->error());
      }

      resetNetworkReply();
      m_mutex->unlock();

      emit requestProcessed(success);
    }

    void OSServer_Impl::processPostAnalysisJSON()
    {
      bool success = false;

      logNetworkReply("processPostAnalysisJSON");

      if (m_networkReply->error() == QNetworkReply::NoError){
        m_lastPostAnalysisJSONSuccess = true;
        success = true;
      }else{
        logNetworkError(m_networkReply->error());
      }

      resetNetworkReply();
      m_mutex->unlock();

      emit requestProcessed(success);
    }

    void OSServer_Impl::processPostDataPointJSON()
    {
      bool success = false;

      logNetworkReply("processPostDataPointJSON");

      if (m_networkReply->error() == QNetworkReply::NoError){
        m_lastPostDataPointJSONSuccess = true;
        success = true;
      }else{
        logNetworkError(m_networkReply->error());
      }

      resetNetworkReply();
      m_mutex->unlock();

      emit requestProcessed(success);
    }

    void OSServer_Impl::processUploadAnalysisFiles()
    {
      bool success = false;

      logNetworkReply("processUploadAnalysisFiles");

      if (m_networkReply->error() == QNetworkReply::NoError){
        m_lastUploadAnalysisFilesSuccess = true;
        success = true;
      }else{
        logNetworkError(m_networkReply->error());
      }

      resetNetworkReply();
      m_mutex->unlock();

      emit requestProcessed(success);     
    }

    void OSServer_Impl::processStart()
    {
      bool success = false;

      logNetworkReply("processStart");

      if (m_networkReply->error() == QNetworkReply::NoError){
        m_lastStartSuccess = true;
        success = true;
      }else{
        logNetworkError(m_networkReply->error());
      }

      resetNetworkReply();
      m_mutex->unlock();

      emit requestProcessed(success);     
    }

    void OSServer_Impl::processIsAnalysisQueued()
    {
      bool success = false;

      logNetworkReply("processIsAnalysisQueued");

      if (m_networkReply->error() == QNetworkReply::NoError){

        QJsonParseError err;
        QJsonDocument json = QJsonDocument::fromJson(m_networkReply->readAll(), &err);
        if (!err.error) {

          if (json.object().contains("analysis")){

            QJsonObject analysis = json.object()["analysis"].toObject();
            if (analysis.contains("status")){

              success = true;
              QString status = analysis["status"].toString();

              // possible status are 'queued', 'started', 'completed'
              if (status == "queued"){
                m_lastIsAnalysisQueued = true;
              }else{
                m_lastIsAnalysisQueued = false;
              }
            }

          }else{
            logError("Incorrect JSON response");
          }

        }else{
          logError("Could not parse JSON response");
        }

      }else{
        logNetworkError(m_networkReply->error());
      }

      resetNetworkReply();
      m_mutex->unlock();

      emit requestProcessed(success);     
    }

    void OSServer_Impl::processIsAnalysisRunning()
    {
      bool success = false;

      logNetworkReply("processIsAnalysisRunning");

      if (m_networkReply->error() == QNetworkReply::NoError){

        QJsonParseError err;
        QJsonDocument json = QJsonDocument::fromJson(m_networkReply->readAll(), &err);

        if (!err.error) {

          if (json.object().contains("analysis")){

            QJsonObject analysis = json.object()["analysis"].toObject();
            if (analysis.contains("status")){

              success = true;
              QString status = analysis["status"].toString();

              // possible status are 'queued', 'started', 'completed'
              if (status == "started"){
                m_lastIsAnalysisRunning = true;
              }else{
                m_lastIsAnalysisRunning = false;
              }
            }

          }else{
            logError("Incorrect JSON response");
          }

        }else{
          logError("Could not parse JSON response");
        }

      }else{
        logNetworkError(m_networkReply->error());
      }

      resetNetworkReply();
      m_mutex->unlock();

      emit requestProcessed(success);     
    }

    void OSServer_Impl::processIsAnalysisComplete()
    {
      bool success = false;

      logNetworkReply("processIsAnalysisComplete");

      if (m_networkReply->error() == QNetworkReply::NoError){

        QJsonParseError err;
        QJsonDocument json = QJsonDocument::fromJson(m_networkReply->readAll(), &err);

        if (!err.error) {

          if (json.object().contains("analysis")){

            QJsonObject analysis = json.object()["analysis"].toObject();
            if (analysis.contains("status")){

              success = true;
              QString status = analysis["status"].toString();

              // possible status are 'queued', 'started', 'completed'
              if (status == "completed"){
                m_lastIsAnalysisComplete = true;
              }else{
                m_lastIsAnalysisComplete = false;
              }
            }

          }else{
            logError("Incorrect JSON response");
          }

        }else{
          logError("Could not parse JSON response");
        }

      }else{
        logNetworkError(m_networkReply->error());
      }

      resetNetworkReply();
      m_mutex->unlock();

      emit requestProcessed(success);
    }

    void OSServer_Impl::processStop()
    {
      bool success = false;

      logNetworkReply("processStop");

      if (m_networkReply->error() == QNetworkReply::NoError){
        m_lastStopSuccess = true;
        success = true;
      }else{
        logNetworkError(m_networkReply->error());
      }

      resetNetworkReply();
      m_mutex->unlock();

      emit requestProcessed(success);
    }

    void OSServer_Impl::processDataPointUUIDs()
    {
      bool success = false;

      logNetworkReply("processDataPointUUIDs");

      if (m_networkReply->error() == QNetworkReply::NoError){

        QJsonParseError err;
        QJsonDocument json = QJsonDocument::fromJson(m_networkReply->readAll(), &err);

        if (!err.error) {

          if (json.object().contains("data_points")){
            
            QJsonArray dataPoints = json.object()["data_points"].toArray();

            m_lastDataPointUUIDs = processListOfUUID(dataPoints, success);
  
          }else{
            logError("Incorrect JSON response");
          }

        }else{
          logError("Could not parse JSON response");
        }
      }else{
        logNetworkError(m_networkReply->error());
      }

      resetNetworkReply();
      m_mutex->unlock();

      emit requestProcessed(success);
    }

    void OSServer_Impl::processQueuedDataPointUUIDs()
    {
      bool success = false;

      logNetworkReply("processQueuedDataPointUUIDs");

      if (m_networkReply->error() == QNetworkReply::NoError){

        QJsonParseError err;
        QJsonDocument json = QJsonDocument::fromJson(m_networkReply->readAll(), &err);

        if (!err.error) {

          if (json.object().contains("data_points")){
            
            QJsonArray dataPoints = json.object()["data_points"].toArray();

            m_lastQueuedDataPointUUIDs = processListOfUUID(dataPoints, success);
  
          }else{
            logError("Incorrect JSON response");
          }

        }else{
          logError("Could not parse JSON response");
        }
      }else{
        logNetworkError(m_networkReply->error());
      }

      resetNetworkReply();
      m_mutex->unlock();

      emit requestProcessed(success);
    }

    void OSServer_Impl::processRunningDataPointUUIDs()
    {
      bool success = false;

      logNetworkReply("processRunningDataPointUUIDs");

      if (m_networkReply->error() == QNetworkReply::NoError){

        QJsonParseError err;
        QJsonDocument json = QJsonDocument::fromJson(m_networkReply->readAll(), &err);

        if (!err.error) {

          if (json.object().contains("data_points")){
            
            QJsonArray dataPoints = json.object()["data_points"].toArray();

            m_lastRunningDataPointUUIDs = processListOfUUID(dataPoints, success);
  
          }else{
            logError("Incorrect JSON response");
          }

        }else{
          logError("Could not parse JSON response");
        }
      }else{
        logNetworkError(m_networkReply->error());
      }

      resetNetworkReply();
      m_mutex->unlock();

      emit requestProcessed(success);
    }

    void OSServer_Impl::processCompleteDataPointUUIDs()
    {
      bool success = false;

      logNetworkReply("processCompleteDataPointUUIDs");

      if (m_networkReply->error() == QNetworkReply::NoError){

        QJsonParseError err;
        QJsonDocument json = QJsonDocument::fromJson(m_networkReply->readAll(), &err);

        if (!err.error) {

          if (json.object().contains("data_points")){
            
            QJsonArray dataPoints = json.object()["data_points"].toArray();

            m_lastCompleteDataPointUUIDs = processListOfUUID(dataPoints, success);
  
          }else{
            logError("Incorrect JSON response");
          }

        }else{
          logError("Could not parse JSON response");
        }
      }else{
        logNetworkError(m_networkReply->error());
      }

      resetNetworkReply();
      m_mutex->unlock();

      emit requestProcessed(success);
    }

    void OSServer_Impl::processDownloadReadyDataPointUUIDs()
    {
      bool success = false;

      logNetworkReply("processDownloadReadyDataPointUUIDs");

      if (m_networkReply->error() == QNetworkReply::NoError){

        QJsonParseError err;
        QJsonDocument json = QJsonDocument::fromJson(m_networkReply->readAll(), &err);

        if (!err.error) {

          if (json.object().contains("data_points")){
            
            QJsonArray dataPoints = json.object()["data_points"].toArray();

            m_lastDownloadReadyDataPointUUIDs = processListOfUUID(dataPoints, success);
  
          }else{
            logError("Incorrect JSON response");
          }

        }else{
          logError("Could not parse JSON response");
        }
      }else{
        logNetworkError(m_networkReply->error());
      }

      resetNetworkReply();
      m_mutex->unlock();

      emit requestProcessed(success);
    }

    void OSServer_Impl::processDataPointJSON()
    {
      bool success = false;

      logNetworkReply("processDataPointJSON");

      if (m_networkReply->error() == QNetworkReply::NoError){
        QJsonParseError err;
        QJsonDocument json = QJsonDocument::fromJson(m_networkReply->readAll(), &err);

        if (!err.error) {
          // DLM: the PAT style datapoint json is underneath results: {pat_data_point:{
          if (json.object().contains("results")){
            QJsonValue results = json.object().value("results");
            if (results.isObject() && results.toObject().contains("pat_data_point")){
              QJsonValue pat_data_point = results.toObject().value("pat_data_point");
              QByteArray pat_json_byte_array = QJsonDocument(pat_data_point.toObject()).toJson(QJsonDocument::Compact);
              QString pat_json(pat_json_byte_array);
              m_lastDataPointJSON = pat_json.toStdString();
              success = true;
            }
          }
        }

        if (!success){
          // some sort of error occurred, potentially the response json does not have pat_data_point in it
          // how to handle this so pat doesn't get stuck?
        }
      }else{
        logNetworkError(m_networkReply->error());
      }

      resetNetworkReply();
      m_mutex->unlock();

      emit requestProcessed(success);
    }
     
    void OSServer_Impl::processDownloadDataPointComplete()
    {
      bool success = false;

      logNetworkReply("processDownloadDataPointComplete");

      if (m_networkReply->error() == QNetworkReply::NoError){
        
        QFile file(toQString(m_lastDownloadDataPointPath));
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)){
          file.write(m_networkReply->readAll());
          m_lastDownloadDataPointSuccess = true;
          success = true;
          file.close();
        } 
        
      }else{
        logNetworkError(m_networkReply->error());
      }

      resetNetworkReply();
      m_mutex->unlock();

      emit requestProcessed(success);
    }

    void OSServer_Impl::processDeleteDataPoint()
    {
      bool success = false;

      logNetworkReply("processDeleteDataPoint");

      if (m_networkReply->error() == QNetworkReply::NoError){
        m_lastDeleteDataPointSuccess = true;
        success = true;
      }else{
        logNetworkError(m_networkReply->error());
      }

      resetNetworkReply();
      m_mutex->unlock();

      emit requestProcessed(success);
    }

    void OSServer_Impl::logUploadProgress(qint64 bytesSent, qint64 bytesTotal) {
      if (bytesTotal == -1) {
        LOG(Debug,"Unknown number of bytes in upload.");
      } else if (bytesTotal == 0) {
          LOG(Debug, "0 bytes in upload.");
      } else {
        double percentComplete = 100.0 * (double(bytesSent)/double(bytesTotal));
        LOG(Debug,"Upload is " << percentComplete << "% complete, " << bytesSent << " of " << bytesTotal << " bytes.");
      }
    }

    void OSServer_Impl::clearErrorsAndWarnings()
    {
      m_errors.clear();
      m_warnings.clear();
    }

    void OSServer_Impl::logNetworkReply(const std::string& methodName) const
    {
      bool doLog = false;

      if (doLog && m_networkReply){
        LogLevel level = Warn;

        LOG(level, methodName);

        QNetworkRequest request = m_networkReply->request();

        LOG(level, "Request Url");
        LOG(level, toString(request.url().toString()));

        LOG(level, "Request Header");
        for(int i=0; i<request.rawHeaderList().size(); ++i){
          QString str(request.rawHeaderList()[i].constData());
          LOG(level, toString(str) << ": " << toString(request.rawHeader(request.rawHeaderList()[i] ).constData()));
        }

        LOG(level, "Reply Header");
        for(int i=0; i<m_networkReply->rawHeaderList().size(); ++i){
          QString str(m_networkReply->rawHeaderList()[i].constData());
          LOG(level, toString(str) << ": " << toString(m_networkReply->rawHeader(m_networkReply->rawHeaderList()[i] ).constData()));
        }

        // DLM: do not call m_networkReply->readAll() here because this will clear the buffer and cannot be reset
      }
    }

    void OSServer_Impl::logError(const std::string& error) const
    {
      m_errors.push_back(error);
      LOG(Error, error);
    }

    void OSServer_Impl::logNetworkError(int error) const
    {
      std::stringstream ss; 
      ss << "Network error '";
      switch (error) {
      case QNetworkReply::NoError:
        ss << "NoError";
        break;
      case QNetworkReply::ConnectionRefusedError:
        ss << "ConnectionRefusedError";
        break;
      case QNetworkReply::RemoteHostClosedError:
        ss << "RemoteHostClosedError";
        break;
      case QNetworkReply::HostNotFoundError:
        ss << "HostNotFoundError";
        break;
      case QNetworkReply::TimeoutError:
        ss << "TimeoutError";
        break;
      case QNetworkReply::OperationCanceledError:
        ss << "OperationCanceledError";
        break;
      case QNetworkReply::SslHandshakeFailedError:
        ss << "SslHandshakeFailedError";
        break;
      case QNetworkReply::TemporaryNetworkFailureError:
        ss << "TemporaryNetworkFailureError";
        break;
      case QNetworkReply::NetworkSessionFailedError:
        ss << "NetworkSessionFailedError";
        break;
      case QNetworkReply::BackgroundRequestNotAllowedError:
        ss << "BackgroundRequestNotAllowedError";
        break;
      case QNetworkReply::ProxyConnectionRefusedError:
        ss << "ProxyConnectionRefusedError";
        break;
      case QNetworkReply::ProxyConnectionClosedError:
        ss << "ProxyConnectionClosedError";
        break;
      case QNetworkReply::ProxyNotFoundError:
        ss << "ProxyNotFoundError";
        break;
      case QNetworkReply::ProxyTimeoutError:
        ss << "ProxyTimeoutError";
        break;
      case QNetworkReply::ProxyAuthenticationRequiredError:
        ss << "ProxyAuthenticationRequiredError";
        break;
      case QNetworkReply::ContentAccessDenied:
        ss << "ContentAccessDenied";
        break;
      case QNetworkReply::ContentOperationNotPermittedError:
        ss << "ContentOperationNotPermittedError";
        break;
      case QNetworkReply::ContentNotFoundError:
        ss << "ContentNotFoundError";
        break;
      case QNetworkReply::AuthenticationRequiredError:
        ss << "AuthenticationRequiredError";
        break;
      case QNetworkReply::ContentReSendError:
        ss << "ContentReSendError";
        break;
        /*case QNetworkReply::ContentConflictError:
        ss << "ContentConflictError";
        break;
        case QNetworkReply::ContentGoneError:
        ss << "ContentGoneError";
        break;
        case QNetworkReply::InternalServerError:
        ss << "InternalServerError";
        break;
        case QNetworkReply::OperationNotImplementedError:
        ss << "OperationNotImplementedError";
        break;
        case QNetworkReply::ServiceUnavailableError:
        ss << "ServiceUnavailableError";
        break;*/
      case QNetworkReply::ProtocolUnknownError:
        ss << "ProtocolUnknownError";
        break;
      case QNetworkReply::ProtocolInvalidOperationError:
        ss << "ProtocolInvalidOperationError";
        break;
      case QNetworkReply::UnknownNetworkError:
        ss << "UnknownNetworkError";
        break;
      case QNetworkReply::UnknownProxyError:
        ss << "UnknownProxyError";
        break;
      case QNetworkReply::UnknownContentError:
        ss << "UnknownContentError";
        break;
      case QNetworkReply::ProtocolFailure:
        ss << "ProtocolFailure";
        break;
        /*case QNetworkReply::UnknownServerError:
        ss << "UnknownServerError";
        break;*/
      default:
        ss << "Unknown";
      }
      ss<< "' occurred";

      m_errors.push_back(ss.str());
      LOG(Error, ss.str());
    }

    void OSServer_Impl::logWarning(const std::string& warning) const
    {
      m_warnings.push_back(warning);
      LOG(Warn, warning);
    }

    std::vector<UUID> OSServer_Impl::processListOfUUID(const QJsonArray& array, bool& success) const
    {
      success = true;

      std::vector<UUID> result;

      for (const QJsonValue& val : array) {
        if (val.toObject().contains("_id")){
          QString id = val.toObject()["_id"].toString();
          UUID uuid(id);
          result.push_back(uuid);
        }else{
          success = false;
          result.clear();
          logError("Incorrect JSON response");
          break;
        }
      }

      return result;
    }

    void OSServer_Impl::resetNetworkReply() {
      if (m_networkReply) {
        m_networkReply->blockSignals(true);
        m_networkReply->deleteLater();
        m_networkReply = nullptr;
      };
    }

  }

  OSServer::OSServer(const QUrl& url)
    : m_impl(std::shared_ptr<detail::OSServer_Impl>(new detail::OSServer_Impl(url)))
  {
  }

  OSServer::~OSServer()
  {
  }

  bool OSServer::available(int msec)
  {
    return getImpl<detail::OSServer_Impl>()->available(msec);
  }

  bool OSServer::lastAvailable() const
  {
    return getImpl<detail::OSServer_Impl>()->lastAvailable();
  }

  std::vector<UUID> OSServer::projectUUIDs(int msec)
  {
    return getImpl<detail::OSServer_Impl>()->projectUUIDs(msec);
  }

  std::vector<UUID> OSServer::lastProjectUUIDs() const
  {
    return getImpl<detail::OSServer_Impl>()->lastProjectUUIDs();
  } 

  bool OSServer::createProject(const UUID& projectUUID, int msec) 
  {
    return getImpl<detail::OSServer_Impl>()->createProject(projectUUID, msec);
  }

  bool OSServer::lastCreateProjectSuccess() const
  {
    return getImpl<detail::OSServer_Impl>()->lastCreateProjectSuccess();
  }

  bool OSServer::deleteProject(const UUID& projectUUID, int msec) 
  {
    return getImpl<detail::OSServer_Impl>()->deleteProject(projectUUID, msec);
  }

  bool OSServer::lastDeleteProjectSuccess() const
  {
    return getImpl<detail::OSServer_Impl>()->lastDeleteProjectSuccess();
  }

  std::vector<UUID> OSServer::analysisUUIDs(const UUID& projectUUID, int msec)
  {
    return getImpl<detail::OSServer_Impl>()->analysisUUIDs(projectUUID, msec);
  }

  std::vector<UUID> OSServer::lastAnalysisUUIDs() const
  {
    return getImpl<detail::OSServer_Impl>()->lastAnalysisUUIDs();
  }

  bool OSServer::postAnalysisJSON(const UUID& projectUUID, const std::string& analysisJSON, int msec)
  {
    return getImpl<detail::OSServer_Impl>()->postAnalysisJSON(projectUUID, analysisJSON, msec);
  }

  bool OSServer::lastPostAnalysisJSONSuccess() const
  {
    return getImpl<detail::OSServer_Impl>()->lastPostAnalysisJSONSuccess();
  }

  bool OSServer::postDataPointJSON(const UUID& analysisUUID, const std::string& dataPointJSON, int msec)
  {
    return getImpl<detail::OSServer_Impl>()->postDataPointJSON(analysisUUID, dataPointJSON, msec);
  }

  bool OSServer::lastPostDataPointJSONSuccess() const
  {
    return getImpl<detail::OSServer_Impl>()->lastPostDataPointJSONSuccess();
  }

  bool OSServer::uploadAnalysisFiles(const UUID& analysisUUID, const openstudio::path& analysisZipFile, int msec)
  {
    return getImpl<detail::OSServer_Impl>()->uploadAnalysisFiles(analysisUUID, analysisZipFile, msec);
  }

  bool OSServer::lastUploadAnalysisFilesSuccess() const
  {
    return getImpl<detail::OSServer_Impl>()->lastUploadAnalysisFilesSuccess();
  }

  bool OSServer::start(const UUID& analysisUUID, int msec)
  {
    return getImpl<detail::OSServer_Impl>()->start(analysisUUID, msec);
  }

  bool OSServer::lastStartSuccess() const
  {
    return getImpl<detail::OSServer_Impl>()->lastStartSuccess();
  }

  bool OSServer::isAnalysisQueued(const UUID& analysisUUID, int msec)
  {
    return getImpl<detail::OSServer_Impl>()->isAnalysisQueued(analysisUUID, msec);
  }

  bool OSServer::lastIsAnalysisQueued() const
  {
    return getImpl<detail::OSServer_Impl>()->lastIsAnalysisQueued();
  }

  bool OSServer::isAnalysisRunning(const UUID& analysisUUID, int msec)
  {
    return getImpl<detail::OSServer_Impl>()->isAnalysisRunning(analysisUUID, msec);
  }

  bool OSServer::lastIsAnalysisRunning() const
  {
    return getImpl<detail::OSServer_Impl>()->lastIsAnalysisRunning();
  }  
  
  bool OSServer::isAnalysisComplete(const UUID& analysisUUID, int msec)
  {
    return getImpl<detail::OSServer_Impl>()->isAnalysisComplete(analysisUUID, msec);
  }

  bool OSServer::lastIsAnalysisComplete() const
  {
    return getImpl<detail::OSServer_Impl>()->lastIsAnalysisComplete();
  }

  bool OSServer::stop(const UUID& analysisUUID, int msec)
  {
    return getImpl<detail::OSServer_Impl>()->stop(analysisUUID, msec);
  }

  bool OSServer::lastStopSuccess() const
  {
    return getImpl<detail::OSServer_Impl>()->lastStopSuccess();
  }

  std::vector<UUID> OSServer::dataPointUUIDs(const UUID& analysisUUID, int msec)
  {
    return getImpl<detail::OSServer_Impl>()->dataPointUUIDs(analysisUUID, msec);
  }

  std::vector<UUID> OSServer::lastDataPointUUIDs() const
  {
    return getImpl<detail::OSServer_Impl>()->lastDataPointUUIDs();
  }

  std::vector<UUID> OSServer::queuedDataPointUUIDs(const UUID& analysisUUID, int msec) 
  {
    return getImpl<detail::OSServer_Impl>()->queuedDataPointUUIDs(analysisUUID, msec);
  }

  std::vector<UUID> OSServer::lastQueuedDataPointUUIDs() const
  {
    return getImpl<detail::OSServer_Impl>()->lastQueuedDataPointUUIDs();
  }

  std::vector<UUID> OSServer::runningDataPointUUIDs(const UUID& analysisUUID, int msec) 
  {
    return getImpl<detail::OSServer_Impl>()->runningDataPointUUIDs(analysisUUID, msec);
  }

  std::vector<UUID> OSServer::lastRunningDataPointUUIDs() const
  {
    return getImpl<detail::OSServer_Impl>()->lastRunningDataPointUUIDs();
  }

  std::vector<UUID> OSServer::completeDataPointUUIDs(const UUID& analysisUUID, int msec)
  {
    return getImpl<detail::OSServer_Impl>()->completeDataPointUUIDs(analysisUUID, msec);
  }

  std::vector<UUID> OSServer::lastCompleteDataPointUUIDs() const
  {
    return getImpl<detail::OSServer_Impl>()->lastCompleteDataPointUUIDs();
  }

  std::vector<UUID> OSServer::downloadReadyDataPointUUIDs(const UUID& analysisUUID, int msec)
  {
    return getImpl<detail::OSServer_Impl>()->downloadReadyDataPointUUIDs(analysisUUID, msec);
  }

  std::vector<UUID> OSServer::lastDownloadReadyDataPointUUIDs() const
  {
    return getImpl<detail::OSServer_Impl>()->lastDownloadReadyDataPointUUIDs();
  }

  std::string OSServer::dataPointJSON(const UUID& analysisUUID, const UUID& dataPointUUID, int msec) 
  {
    return getImpl<detail::OSServer_Impl>()->dataPointJSON(analysisUUID, dataPointUUID, msec);
  }

  std::string OSServer::lastDataPointJSON() const
  {
    return getImpl<detail::OSServer_Impl>()->lastDataPointJSON();
  }

  bool OSServer::downloadDataPoint(const UUID& analysisUUID, const UUID& dataPointUUID, const openstudio::path& downloadPath, int msec) 
  {
    return getImpl<detail::OSServer_Impl>()->downloadDataPoint(analysisUUID, dataPointUUID, downloadPath, msec);
  }

  bool OSServer::lastDownloadDataPointSuccess() const
  {
    return getImpl<detail::OSServer_Impl>()->lastDownloadDataPointSuccess();
  }

  bool OSServer::deleteDataPoint(const UUID& analysisUUID, const UUID& dataPointUUID, int msec)
  {
    return getImpl<detail::OSServer_Impl>()->deleteDataPoint(analysisUUID,dataPointUUID,msec);
  }

  bool OSServer::lastDeleteDataPointSuccess() const {
    return getImpl<detail::OSServer_Impl>()->lastDeleteDataPointSuccess();
  }

  bool OSServer::waitForFinished(int msec) 
  {
    return getImpl<detail::OSServer_Impl>()->waitForFinished(msec);
  }

  std::vector<std::string> OSServer::errors() const
  {
    return getImpl<detail::OSServer_Impl>()->errors();
  }

  std::vector<std::string> OSServer::warnings() const
  {
    return getImpl<detail::OSServer_Impl>()->warnings();
  }

  bool OSServer::requestAvailable()
  {
    return getImpl<detail::OSServer_Impl>()->requestAvailable();
  }

  bool OSServer::requestProjectUUIDs()
  {
    return getImpl<detail::OSServer_Impl>()->requestProjectUUIDs();
  } 

  bool OSServer::requestCreateProject(const UUID& projectUUID) 
  {
    return getImpl<detail::OSServer_Impl>()->requestCreateProject(projectUUID);
  }

  bool OSServer::requestDeleteProject(const UUID& projectUUID) 
  {
    return getImpl<detail::OSServer_Impl>()->requestDeleteProject(projectUUID);
  }

  bool OSServer::requestAnalysisUUIDs(const UUID& projectUUID)
  {
    return getImpl<detail::OSServer_Impl>()->requestAnalysisUUIDs(projectUUID);
  }

  bool OSServer::startPostAnalysisJSON(const UUID& projectUUID, const std::string& analysisJSON) 
  {
    return getImpl<detail::OSServer_Impl>()->startPostAnalysisJSON(projectUUID, analysisJSON);
  }

  bool OSServer::startPostDataPointJSON(const UUID& analysisUUID, const std::string& dataPointJSON) 
  {
    return getImpl<detail::OSServer_Impl>()->startPostDataPointJSON(analysisUUID, dataPointJSON);
  }

  bool OSServer::startUploadAnalysisFiles(const UUID& analysisUUID, const openstudio::path& analysisZipFile) 
  {
    return getImpl<detail::OSServer_Impl>()->startUploadAnalysisFiles(analysisUUID, analysisZipFile);
  }

  bool OSServer::requestStart(const UUID& analysisUUID) 
  {
    return getImpl<detail::OSServer_Impl>()->requestStart(analysisUUID);
  }

  bool OSServer::requestIsAnalysisQueued(const UUID& analysisUUID) 
  {
    return getImpl<detail::OSServer_Impl>()->requestIsAnalysisQueued(analysisUUID);
  }

  bool OSServer::requestIsAnalysisRunning(const UUID& analysisUUID) 
  {
    return getImpl<detail::OSServer_Impl>()->requestIsAnalysisRunning(analysisUUID);
  }

  bool OSServer::requestIsAnalysisComplete(const UUID& analysisUUID) 
  {
    return getImpl<detail::OSServer_Impl>()->requestIsAnalysisComplete(analysisUUID);
  }

  bool OSServer::requestStop(const UUID& analysisUUID) 
  {
    return getImpl<detail::OSServer_Impl>()->requestStop(analysisUUID);
  }

  bool OSServer::requestDataPointUUIDs(const UUID& analysisUUID) 
  {
    return getImpl<detail::OSServer_Impl>()->requestDataPointUUIDs(analysisUUID);
  }

  bool OSServer::requestQueuedDataPointUUIDs(const UUID& analysisUUID) 
  {
    return getImpl<detail::OSServer_Impl>()->requestQueuedDataPointUUIDs(analysisUUID);
  }

  bool OSServer::requestRunningDataPointUUIDs(const UUID& analysisUUID) 
  {
    return getImpl<detail::OSServer_Impl>()->requestRunningDataPointUUIDs(analysisUUID);
  }

  bool OSServer::requestCompleteDataPointUUIDs(const UUID& analysisUUID) 
  {
    return getImpl<detail::OSServer_Impl>()->requestCompleteDataPointUUIDs(analysisUUID);
  }

  bool OSServer::requestDownloadReadyDataPointUUIDs(const UUID& analysisUUID) 
  {
    return getImpl<detail::OSServer_Impl>()->requestDownloadReadyDataPointUUIDs(analysisUUID);
  }

  bool OSServer::requestDataPointJSON(const UUID& analysisUUID, const UUID& dataPointUUID) 
  {
    return getImpl<detail::OSServer_Impl>()->requestDataPointJSON(analysisUUID, dataPointUUID);
  }

  bool OSServer::startDownloadDataPoint(const UUID& analysisUUID, const UUID& dataPointUUID, const openstudio::path& downloadPath) 
  {
    return getImpl<detail::OSServer_Impl>()->startDownloadDataPoint(analysisUUID, dataPointUUID, downloadPath);
  }

  bool OSServer::requestDeleteDataPoint(const UUID& analysisUUID, const UUID& dataPointUUID) {
    return getImpl<detail::OSServer_Impl>()->requestDeleteDataPoint(analysisUUID,dataPointUUID);
  }

  bool OSServer::connect(const char* signal,
                         const QObject* qObject,
                         const char* slot,
                         Qt::ConnectionType type) const
  {
    return QObject::connect(getImpl<detail::OSServer_Impl>().get(), signal, qObject, slot, type);
  }

  bool OSServer::disconnect(const char* signal,
                            const QObject* receiver,
                            const char* slot) const
  {
    return QObject::disconnect(getImpl<detail::OSServer_Impl>().get(), signal, receiver, slot);
  }

} // openstudio
