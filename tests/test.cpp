/**
 * Copyright 2025 Mo-Sys Engineering Ltd
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), 
 * to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, 
 * distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <nlohmann/json.hpp>
#include <nlohmann/json-schema.hpp>
#include <opentrackio-cpp/OpenTrackIOSample.h>
#include <span>

using nlohmann::json;
using nlohmann::json_schema::json_validator;

namespace
{
    // Store reletive paths to testdata.
    const std::filesystem::path SMPTE_METADATA_CAMDKIT_ROOT = "testdata";
    const std::filesystem::path SMPTE_EXAMPLES_ROOT = SMPTE_METADATA_CAMDKIT_ROOT / "examples";
} // namespace

inline bool readData(std::filesystem::path path, std::string& data) 
{
    std::ifstream fileHandle(path);
    if (!fileHandle.is_open())
        return false;

    fileHandle.seekg(0, fileHandle.end);
    auto length = fileHandle.tellg();
    fileHandle.seekg(0, fileHandle.beg);
    data.clear();
    data.resize(length);
    fileHandle.read(data.data(), length);
    return true;
}

void checkStructureForErrors(const std::vector<std::string>& errors)
{
    std::string errorMsg = "Errors reported!: \n";
    for (const auto& errorStr : errors)
    {
        errorMsg += (errorStr + '\n');
    }
    INFO(errorMsg);
    REQUIRE(errors.size() == 0);
}

bool getStringExample(const std::string sampleName, std::string& data) 
{
    std::filesystem::path targetPath = SMPTE_EXAMPLES_ROOT / (sampleName + ".json");
    return readData(targetPath, data);
}

bool getStringSchema(std::string& data) 
{
    std::filesystem::path targetPath = (SMPTE_METADATA_CAMDKIT_ROOT / "schema.json");
    return readData(targetPath, data);
}

TEST_CASE("OpenTrackIOSample basic initialisation", "[init]")
{
    SECTION("Initialising from an empty string should be unsuccessful.")
    {
        opentrackio::OpenTrackIOSample sample;
        CHECK_FALSE(sample.initialise(std::string_view("")));
    }

    SECTION("Initialising from an empty CBOR object should be unsuccessful.")
    {
        opentrackio::OpenTrackIOSample sample;
        std::span<const uint8_t> cbor;
        CHECK_FALSE(sample.initialise(cbor));
    }

    SECTION("Initialising from an empty JSON object should be unsuccessful.")
    {
        opentrackio::OpenTrackIOSample sample;
        json j;

        REQUIRE_FALSE(sample.initialise(std::string_view("")));
        CHECK_FALSE(sample.getErrors().empty());
        CHECK(sample.getWarnings().empty());
        CHECK(sample.getJson() == "null");
    }
}

void testVersion(const std::vector<uint16_t>& version)
{
    REQUIRE(version.size() == 3);
    REQUIRE(version[0] == OPEN_TRACK_IO_PROTOCOL_MAJOR_VERSION);
    REQUIRE(version[1] == OPEN_TRACK_IO_PROTOCOL_MINOR_VERSION);
    REQUIRE(version[2] == OPEN_TRACK_IO_PROTOCOL_PATCH);
}

void testSampleParse(const std::string& response, opentrackio::OpenTrackIOSample& sample)
{
    bool initialised = sample.initialise(std::string_view(response));
    checkStructureForErrors(sample.getErrors());
    REQUIRE(initialised);
    REQUIRE(sample.protocol->name == OPEN_TRACK_IO_PROTOCOL_NAME);
    testVersion(sample.protocol->version);
}

void testRecommendedDynamic(const std::string& response)
{
    opentrackio::OpenTrackIOSample sample;
    testSampleParse(response, sample);
    checkStructureForErrors(sample.getErrors());

    CHECK(sample.tracker->notes == "Example generated sample.");
    CHECK(sample.tracker->recording == false);
    CHECK(sample.tracker->slate == "A101_A_4");
    CHECK(sample.tracker->status == "Optical Good");

    CHECK(sample.timing->mode == opentrackio::opentrackioproperties::Timing::Mode::EXTERNAL);
    CHECK(sample.timing->sampleRate->numerator == 24);
    CHECK(sample.timing->sampleRate->denominator == 1);
    CHECK(sample.timing->timecode->hours == 1);
    CHECK(sample.timing->timecode->minutes == 2);
    CHECK(sample.timing->timecode->seconds == 3);
    CHECK(sample.timing->timecode->frames == 4);
    CHECK(sample.timing->timecode->frameRate.numerator == 24);
    CHECK(sample.timing->timecode->frameRate.denominator == 1);
    CHECK_FALSE(sample.timing->timecode->subFrame.has_value());
    CHECK_FALSE(sample.timing->timecode->dropFrame.has_value());

    CHECK(sample.lens->distortion->size() == 1);
    CHECK(sample.lens->distortion->at(0).radial == std::vector<double>{1.0, 2.0, 3.0});
    CHECK(sample.lens->distortion->at(0).tangential == std::vector<double>{1.0, 2.0});
    CHECK(sample.lens->distortion->at(0).overscan == 3.1);
    CHECK(sample.lens->encoders->focus == 0.1);
    CHECK(sample.lens->encoders->iris == 0.2);
    CHECK(sample.lens->encoders->zoom == 0.3);
    CHECK(sample.lens->entrancePupilOffset == 0.123);
    CHECK(sample.lens->fStop == 4.0);
    CHECK(sample.lens->pinholeFocalLength == 24.305);
    CHECK(sample.lens->focusDistance == 10.0);
    CHECK(sample.lens->projectionOffset->x == 0.1);
    CHECK(sample.lens->projectionOffset->y == 0.2);

    CHECK(sample.protocol->name == OPEN_TRACK_IO_PROTOCOL_NAME);
    testVersion(sample.protocol->version);

    CHECK(sample.sampleId->id.substr(0, 9) == "urn:uuid:");
    CHECK(sample.sourceId->id.substr(0, 9) == "urn:uuid:");
    CHECK(sample.sourceNumber->value == 1);

    CHECK(sample.transforms->transforms.size() == 1);
    CHECK(sample.transforms->transforms[0].translation.x == 1.0);
    CHECK(sample.transforms->transforms[0].translation.y == 2.0);
    CHECK(sample.transforms->transforms[0].translation.z == 3.0);
    CHECK(sample.transforms->transforms[0].rotation.pan == 180.0);
    CHECK(sample.transforms->transforms[0].rotation.tilt == 90.0);
    CHECK(sample.transforms->transforms[0].rotation.roll == 45.0);
    CHECK(sample.transforms->transforms[0].id == "Camera");
}

void testRecommendedStatic(const std::string& response)
{
    opentrackio::OpenTrackIOSample sample;
    testSampleParse(response, sample);

    CHECK(sample.camera->activeSensorPhysicalDimensions->height == 24.0);
    CHECK(sample.camera->activeSensorPhysicalDimensions->width == 36.0);
    CHECK(sample.camera->label == "A");
    CHECK(sample.lens->make == "LensMaker");
    CHECK(sample.lens->model == "Model15");
}

void testCompleteDynamic(const std::string& response)
{
    opentrackio::OpenTrackIOSample sample;
    testSampleParse(response, sample);

    CHECK(sample.tracker->notes == "Example generated sample.");
    CHECK(sample.tracker->recording == false);
    CHECK(sample.tracker->slate == "A101_A_4");
    CHECK(sample.tracker->status == "Optical Good");

    CHECK(sample.timing->mode == opentrackio::opentrackioproperties::Timing::Mode::INTERNAL);
    CHECK(sample.timing->recordedTimestamp->seconds == 1718806000);
    CHECK(sample.timing->recordedTimestamp->nanoseconds == 500000000);
    CHECK(sample.timing->sampleRate->numerator == 24);
    CHECK(sample.timing->sampleRate->denominator == 1);
    CHECK(sample.timing->sampleTimestamp->seconds == 1718806554);
    CHECK(sample.timing->sampleTimestamp->nanoseconds == 500000000);
    CHECK(sample.timing->sequenceNumber == 0);
    CHECK(sample.timing->synchronization->locked);
    CHECK(sample.timing->synchronization->source == opentrackio::opentrackioproperties::Timing::Synchronization::SourceType::PTP);
    CHECK(sample.timing->synchronization->frequency->numerator == 24'000);
    CHECK(sample.timing->synchronization->frequency->denominator == 1'001);
    CHECK(sample.timing->synchronization->present);
    CHECK(sample.timing->synchronization->ptp->profile == opentrackio::opentrackioproperties::Timing::Synchronization::Ptp::ProfileType::SMPTE_ST2059_2_2021);
    CHECK(sample.timing->synchronization->ptp->domain == 1);
    CHECK(sample.timing->synchronization->ptp->leaderIdentity == "00:11:22:33:44:55");
    CHECK(sample.timing->synchronization->ptp->leaderPriorities.priority1 == 128);
    CHECK(sample.timing->synchronization->ptp->leaderPriorities.priority2 == 128);
    CHECK(sample.timing->synchronization->ptp->leaderAccuracy == 5e-08);
    CHECK(sample.timing->synchronization->ptp->leaderTimeSource == opentrackio::opentrackioproperties::Timing::Synchronization::Ptp::LeaderTimeSourceType::GNSS);
    CHECK(sample.timing->synchronization->ptp->meanPathDelay == 0.000123);
    CHECK(sample.timing->synchronization->ptp->vlan == 100);
    CHECK(sample.timing->timecode->hours == 1);
    CHECK(sample.timing->timecode->minutes == 2);
    CHECK(sample.timing->timecode->seconds == 3);
    CHECK(sample.timing->timecode->frames == 4);
    CHECK(sample.timing->timecode->frameRate.numerator == 24'000);
    CHECK(sample.timing->timecode->frameRate.denominator == 1'001);
    CHECK(sample.timing->timecode->subFrame == 1);
    CHECK(sample.timing->timecode->dropFrame == true);

    CHECK(sample.lens->custom->size() == 2);
    CHECK(sample.lens->custom == std::vector<double>{1.0, 2.0});
    CHECK(sample.lens->distortion->size() == 2);
    CHECK(sample.lens->distortion->at(0).model == "Brown-Conrady U-D");
    CHECK(sample.lens->distortion->at(0).radial == std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
    CHECK(sample.lens->distortion->at(0).tangential == std::vector<double>{1.0, 2.0});
    CHECK(sample.lens->distortion->at(0).overscan == 3.0);
    CHECK(sample.lens->distortion->at(1).radial == std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
    CHECK(sample.lens->distortion->at(1).tangential == std::vector<double>{1.0, 2.0});
    CHECK(sample.lens->distortion->at(1).overscan == 2.0);
    CHECK(sample.lens->distortionOffset->x == 1.0);
    CHECK(sample.lens->distortionOffset->y == 2.0);
    CHECK(sample.lens->encoders->focus == 0.1);
    CHECK(sample.lens->encoders->iris == 0.2);
    CHECK(sample.lens->encoders->zoom == 0.3);
    CHECK(sample.lens->entrancePupilOffset == 0.123);
    CHECK(sample.lens->exposureFalloff->a1 == 1.0);
    CHECK(sample.lens->exposureFalloff->a2 == 2.0);
    CHECK(sample.lens->exposureFalloff->a3 == 3.0);
    CHECK(sample.lens->fStop == 4.0);
    CHECK(sample.lens->pinholeFocalLength == 24.305);
    CHECK(sample.lens->focusDistance == 10.0);
    CHECK(sample.lens->projectionOffset->x == 0.1);
    CHECK(sample.lens->projectionOffset->y == 0.2);
    CHECK(sample.lens->rawEncoders->focus == 1000);
    CHECK(sample.lens->rawEncoders->iris == 2000);
    CHECK(sample.lens->rawEncoders->zoom == 3000);
    CHECK(sample.lens->tStop == 4.1);

    CHECK(sample.protocol->name == OPEN_TRACK_IO_PROTOCOL_NAME);
    testVersion(sample.protocol->version);

    CHECK(sample.sourceId->id.substr(0, 9) == "urn:uuid:");
    CHECK(sample.sampleId->id.substr(0, 9) == "urn:uuid:");
    CHECK(sample.sourceNumber->value == 1);
    CHECK(sample.relatedSampleIds->samples.size() == 2);
    CHECK(sample.relatedSampleIds->samples[0].substr(0, 9) == "urn:uuid:");
    CHECK(sample.relatedSampleIds->samples[1].substr(0, 9) == "urn:uuid:");

    CHECK(sample.globalStage->e == 100.0);
    CHECK(sample.globalStage->n == 200.0);
    CHECK(sample.globalStage->u == 300.0);
    CHECK(sample.globalStage->lat0 == 100.0);
    CHECK(sample.globalStage->lon0 == 200.0);
    CHECK(sample.globalStage->h0 == 300.0);

    CHECK(sample.transforms->transforms.size() == 3);
    CHECK(sample.transforms->transforms[0].translation.x == 1.0);
    CHECK(sample.transforms->transforms[0].translation.y == 2.0);
    CHECK(sample.transforms->transforms[0].translation.z == 3.0);
    CHECK(sample.transforms->transforms[0].rotation.pan == 180.0);
    CHECK(sample.transforms->transforms[0].rotation.tilt == 90.0);
    CHECK(sample.transforms->transforms[0].rotation.roll == 45.0);
    CHECK(sample.transforms->transforms[0].id == "Dolly");

    CHECK(sample.transforms->transforms[1].translation.x == 1.0);
    CHECK(sample.transforms->transforms[1].translation.y == 2.0);
    CHECK(sample.transforms->transforms[1].translation.z == 3.0);
    CHECK(sample.transforms->transforms[1].rotation.pan == 180.0);
    CHECK(sample.transforms->transforms[1].rotation.tilt == 90.0);
    CHECK(sample.transforms->transforms[1].rotation.roll == 45.0);
    CHECK(sample.transforms->transforms[1].scale->x == 1.0);
    CHECK(sample.transforms->transforms[1].scale->y == 2.0);
    CHECK(sample.transforms->transforms[1].scale->z == 3.0);
    CHECK(sample.transforms->transforms[1].id == "Crane Arm");

    CHECK(sample.transforms->transforms[2].translation.x == 1.0);
    CHECK(sample.transforms->transforms[2].translation.y == 2.0);
    CHECK(sample.transforms->transforms[2].translation.z == 3.0);
    CHECK(sample.transforms->transforms[2].rotation.pan == 180.0);
    CHECK(sample.transforms->transforms[2].rotation.tilt == 90.0);
    CHECK(sample.transforms->transforms[2].rotation.roll == 45.0);
    CHECK(sample.transforms->transforms[2].scale->x == 1.0);
    CHECK(sample.transforms->transforms[2].scale->y == 2.0);
    CHECK(sample.transforms->transforms[2].scale->z == 3.0);
    CHECK(sample.transforms->transforms[2].id == "Camera");
}

void testCompleteStatic(const std::string& response)
{
    opentrackio::OpenTrackIOSample sample;
    testSampleParse(response, sample);

    // Static properties.
    CHECK(sample.duration->rational.numerator == 1);
    CHECK(sample.duration->rational.denominator == 25);
    CHECK(sample.camera->captureFrameRate->numerator == 24'000);
    CHECK(sample.camera->captureFrameRate->denominator == 1'001);
    CHECK(sample.camera->activeSensorResolution->height == 2160);
    CHECK(sample.camera->activeSensorResolution->width == 3840);
    CHECK(sample.camera->anamorphicSqueeze->numerator == 1);
    CHECK(sample.camera->anamorphicSqueeze->denominator == 1);
    CHECK(sample.camera->make == "CameraMaker");
    CHECK(sample.camera->model == "Model20");
    CHECK(sample.camera->serialNumber == "1234567890A");
    CHECK(sample.camera->firmwareVersion == "1.2.3");
    CHECK(sample.camera->label == "A");
    CHECK(sample.camera->anamorphicSqueeze->numerator == 1);
    CHECK(sample.camera->anamorphicSqueeze->denominator == 1);
    CHECK(sample.camera->isoSpeed == 4'000);
    CHECK(sample.camera->fdlLink->substr(0, 9) == "urn:uuid:");
    CHECK(sample.camera->shutterAngle == 45.0);

    CHECK(sample.duration->rational.numerator == 1);
    CHECK(sample.duration->rational.denominator == 25);

    CHECK(sample.lens->distortionOverscanMax == 1.2);
    CHECK(sample.lens->undistortionOverscanMax == 1.3);
    CHECK(sample.lens->nominalFocalLength == 14);
    CHECK(sample.lens->serialNumber == "1234567890A");

    CHECK(sample.timing->synchronization->locked == true);
    CHECK(sample.timing->synchronization->source == opentrackio::opentrackioproperties::Timing::Synchronization::SourceType::PTP);
    CHECK(sample.timing->synchronization->frequency->numerator == 24'000);
    CHECK(sample.timing->synchronization->frequency->denominator == 1001);
    CHECK(sample.timing->synchronization->present == true);

    CHECK(sample.timing->synchronization->ptp->profile == opentrackio::opentrackioproperties::Timing::Synchronization::Ptp::ProfileType::SMPTE_ST2059_2_2021);
    CHECK(sample.timing->synchronization->ptp->domain == 1);
    CHECK(sample.timing->synchronization->ptp->leaderIdentity == "00:11:22:33:44:55");
    CHECK(sample.timing->synchronization->ptp->leaderPriorities.priority1 == 128);
    CHECK(sample.timing->synchronization->ptp->leaderPriorities.priority2 == 128);
    CHECK(sample.timing->synchronization->ptp->leaderAccuracy == 5e-08);
    CHECK(sample.timing->synchronization->ptp->meanPathDelay == 0.000123);
    CHECK(sample.timing->synchronization->ptp->vlan == 100);
    CHECK(sample.timing->synchronization->ptp->leaderTimeSource == opentrackio::opentrackioproperties::Timing::Synchronization::Ptp::LeaderTimeSourceType::GNSS);

    CHECK(sample.timing->timecode->hours == 1);
    CHECK(sample.timing->timecode->minutes == 2);
    CHECK(sample.timing->timecode->seconds == 3);
    CHECK(sample.timing->timecode->frames == 4);
    CHECK(sample.timing->timecode->frameRate.numerator == 24'000);
    CHECK(sample.timing->timecode->frameRate.denominator == 1001);
    CHECK(sample.timing->timecode->subFrame == 1);
    CHECK(sample.timing->timecode->dropFrame == true);

    CHECK(sample.tracker->notes == "Example generated sample.");
    CHECK(sample.tracker->recording == false);
    CHECK(sample.tracker->slate == "A101_A_4");
    CHECK(sample.tracker->status == "Optical Good");
    CHECK(sample.tracker->firmwareVersion == "1.2.3");
    CHECK(sample.tracker->make == "TrackerMaker");
    CHECK(sample.tracker->model == "Tracker");
    CHECK(sample.tracker->serialNumber == "1234567890A");

    CHECK(sample.transforms->transforms.size() == 3);
    CHECK(sample.transforms->transforms[0].translation.x == 1.0);
    CHECK(sample.transforms->transforms[0].translation.y == 2.0);
    CHECK(sample.transforms->transforms[0].translation.z == 3.0);
    CHECK(sample.transforms->transforms[0].rotation.pan == 180.0);
    CHECK(sample.transforms->transforms[0].rotation.tilt == 90.0);
    CHECK(sample.transforms->transforms[0].rotation.roll == 45.0);
    CHECK(sample.transforms->transforms[0].id == "Dolly");
    CHECK(sample.transforms->transforms[1].translation.x == 1.0);
    CHECK(sample.transforms->transforms[1].translation.y == 2.0);
    CHECK(sample.transforms->transforms[1].translation.z == 3.0);
    CHECK(sample.transforms->transforms[1].rotation.pan == 180.0);
    CHECK(sample.transforms->transforms[1].rotation.tilt == 90.0);
    CHECK(sample.transforms->transforms[1].rotation.roll == 45.0);
    CHECK(sample.transforms->transforms[1].scale->x == 1.0);
    CHECK(sample.transforms->transforms[1].scale->y == 2.0);
    CHECK(sample.transforms->transforms[1].scale->z == 3.0);
    CHECK(sample.transforms->transforms[1].id == "Crane Arm");
    CHECK(sample.transforms->transforms[2].translation.x == 1.0);
    CHECK(sample.transforms->transforms[2].translation.y == 2.0);
    CHECK(sample.transforms->transforms[2].translation.z == 3.0);
    CHECK(sample.transforms->transforms[2].rotation.pan == 180.0);
    CHECK(sample.transforms->transforms[2].rotation.tilt == 90.0);
    CHECK(sample.transforms->transforms[2].rotation.roll == 45.0);
    CHECK(sample.transforms->transforms[2].scale->x == 1.0);
    CHECK(sample.transforms->transforms[2].scale->y == 2.0);
    CHECK(sample.transforms->transforms[2].scale->z == 3.0);
    CHECK(sample.transforms->transforms[2].id == "Camera");
}

TEST_CASE("OpenTrackIOSample example initialisation", "[init]")
{
    std::string response;
    REQUIRE(getStringExample("recommended_dynamic_example", response));
    testRecommendedDynamic(response);

    REQUIRE(getStringExample("recommended_static_example", response));
    testRecommendedDynamic(response);
    testRecommendedStatic(response);

    REQUIRE(getStringExample("complete_dynamic_example", response));
    testCompleteDynamic(response);

    REQUIRE(getStringExample("complete_static_example", response));
    testCompleteDynamic(response);
    testRecommendedStatic(response);
    testCompleteStatic(response);
}

TEST_CASE("OpenTrackIOSamples validate against the published schema", "[validate]")
{
    std::string schemaJson;
    REQUIRE(getStringSchema(schemaJson));
    json schema = json::parse(schemaJson);
    json_validator validator;
    REQUIRE_NOTHROW(validator.set_root_schema(schema));

    for (auto name : {
        "recommended_dynamic_example",
        "recommended_static_example",
        "complete_dynamic_example",
        "complete_static_example",
        })
    {
        INFO(name << " - Output from raw string.");
        std::string data;
        REQUIRE(getStringExample(name, data));
        json baseline = json::parse(data);
        REQUIRE_NOTHROW(validator.validate(baseline));

        INFO(name << " - Output from library.");
        opentrackio::OpenTrackIOSample sample;
        sample.initialise(std::string_view(data));
        checkStructureForErrors(sample.getErrors());
        REQUIRE(sample.getErrors().size() == 0);
        json j = json::parse(sample.getJson());
        CHECK_NOTHROW(validator.validate(j));
    }
}
