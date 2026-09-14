plugins {
    id("com.android.application")
    kotlin("android")
}

android {
    namespace = "io.goreecloud.browser"
    compileSdk = 35

    defaultConfig {
        applicationId = "io.goreecloud.browser"
        minSdk = 26
        targetSdk = 35
        versionCode = 10005
        versionName = "0.1.0-beta.1"

        testInstrumentationRunner = "android.app.Instrumentation"
    }

    buildTypes {
        getByName("debug") {
            // The CI beta is intentionally a distinct application so it can
            // coexist with a future production-signed GoreeCloud Browser.
            applicationIdSuffix = ".beta"
            versionNameSuffix = "+android.5"
        }

        getByName("release") {
            isMinifyEnabled = false
            // Production release signing is intentionally not stored in source
            // control. Production APK/AAB acceptance remains a separate gate.
            signingConfig = null
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    kotlinOptions {
        jvmTarget = "17"
    }

    buildFeatures {
        buildConfig = true
    }

    lint {
        abortOnError = true
        checkReleaseBuilds = true
    }
}

dependencies {
    testImplementation("junit:junit:4.13.2")
}

// Android Beta 0.1 is a real installable test surface, but not a production
// approval. The Android System WebView/Chromium runtime is an engine dependency;
// GoreeCloud owns the mobile chrome, navigation policy, privacy defaults, and
// product behavior. Production signing, complete Glaze UI V1.4 native-device
// acceptance, Wardveil download release, private browsing isolation,
// updates/rollback, and real-device sustained-use evidence remain explicit
// promotion gates.
