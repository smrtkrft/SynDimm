/**
 * SK_lang.h
 * SmartKraft SynDimm - Language Management System
 * Version: v1.1.1
 * 
 * ========================================
 * Multi-Language Support
 * ========================================
 * - EN: English (Default)
 * - DE: German (Deutsch)
 * - TR: Turkish (Türkçe)
 * - Dynamic language switching via API
 * - Persistent language selection
 * ========================================
 */

#ifndef SK_LANG_H
#define SK_LANG_H

#include <Arduino.h>
#include <Preferences.h>
#include "SK_config.h"

// Include language files
#include "SK_lang_en.h"
#include "SK_lang_de.h"
#include "SK_lang_tr.h"

// Language codes
enum LangCode {
    LANG_EN = 0,  // English (Default)
    LANG_DE = 1,  // German
    LANG_TR = 2   // Turkish
};

// Language preferences storage
Preferences langPrefs;

// Current language
LangCode currentLang = LANG_EN;

// Initialize language system
void initLanguage() {
    langPrefs.begin("lang", true);  // Read-only first
    currentLang = (LangCode)langPrefs.getUChar("lang", LANG_EN);
    langPrefs.end();
    DEBUG_PRINTF("[LANG] Loaded language: %s\n", 
        currentLang == LANG_EN ? "EN" : 
        currentLang == LANG_DE ? "DE" : "TR");
}

// Save language preference
void saveLanguage(LangCode lang) {
    currentLang = lang;
    langPrefs.begin("lang", false);
    langPrefs.putUChar("lang", (uint8_t)lang);
    langPrefs.end();
    DEBUG_PRINTF("[LANG] Saved language: %s\n", 
        lang == LANG_EN ? "EN" : 
        lang == LANG_DE ? "DE" : "TR");
}

// Set language from string code
bool setLanguageFromCode(const String& code) {
    LangCode newLang;
    if (code == "en") newLang = LANG_EN;
    else if (code == "de") newLang = LANG_DE;
    else if (code == "tr") newLang = LANG_TR;
    else return false;
    
    saveLanguage(newLang);
    return true;
}

// Get current language code as string
String getCurrentLangCode() {
    switch (currentLang) {
        case LANG_EN: return "en";
        case LANG_DE: return "de";
        case LANG_TR: return "tr";
        default: return "en";
    }
}

// Get language JSON by code
const char* getLanguageJSON(const String& code) {
    if (code == "en") return LANG_EN_JSON;
    if (code == "de") return LANG_DE_JSON;
    if (code == "tr") return LANG_TR_JSON;
    return LANG_EN_JSON;  // Default to English
}

// Get current language JSON
const char* getCurrentLanguageJSON() {
    switch (currentLang) {
        case LANG_EN: return LANG_EN_JSON;
        case LANG_DE: return LANG_DE_JSON;
        case LANG_TR: return LANG_TR_JSON;
        default: return LANG_EN_JSON;
    }
}

// Reset language to default (English)
void resetLanguage() {
    saveLanguage(LANG_EN);
}

#endif // SK_LANG_H
