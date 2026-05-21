// ─────────────────────────────────────────
//  EchoNav — android/screens/SettingsScreen.js
//  Lets the user adjust:
//   - Alert sensitivity (distance thresholds)
//   - Vibration intensity
//   - Audio volume
//   - Language / voice
//   - Blind mode (larger text + audio only)
// ─────────────────────────────────────────

import React, { useState } from 'react';
import {
    View, Text, Switch, Slider,
    StyleSheet, ScrollView, TouchableOpacity
} from 'react-native';
import Tts from 'react-native-tts';


export default function SettingsScreen() {

    // ── Settings state ────────────────────
    const [blindMode,       setBlindMode]       = useState(false);
    const [audioEnabled,    setAudioEnabled]    = useState(true);
    const [vibrationEnabled,setVibrationEnabled]= useState(true);
    const [sensitivity,     setSensitivity]     = useState(1.0);
    const [vibIntensity,    setVibIntensity]    = useState(0.8);
    const [audioVolume,     setAudioVolume]     = useState(0.8);
    const [language,        setLanguage]        = useState('en-US');

    const LANGUAGES = ['en-US', 'fr-FR', 'es-ES', 'ar-SA', 'hi-IN'];


    // ── Announce setting change via TTS ───
    const announce = (msg) => {
        if (audioEnabled) Tts.speak(msg);
    };


    // ── Render a settings row ─────────────
    const SettingRow = ({ label, description, children }) => (
        <View style={styles.settingRow}>
            <View style={styles.settingInfo}>
                <Text style={styles.settingLabel}>{label}</Text>
                {description && (
                    <Text style={styles.settingDesc}>{description}</Text>
                )}
            </View>
            {children}
        </View>
    );


    // ── Render ────────────────────────────
    return (
        <ScrollView style={styles.container}>

            {/* Section: Accessibility */}
            <Text style={styles.section}>Accessibility</Text>

            <SettingRow
                label="Blind Mode"
                description="Larger text, audio-only alerts"
            >
                <Switch
                    value={blindMode}
                    onValueChange={(v) => {
                        setBlindMode(v);
                        announce(v ? 'Blind mode on' : 'Blind mode off');
                    }}
                    trackColor={{ true: '#4a90e2' }}
                />
            </SettingRow>


            {/* Section: Alerts */}
            <Text style={styles.section}>Alerts</Text>

            <SettingRow
                label="Audio Alerts"
                description="Bone conduction beeps"
            >
                <Switch
                    value={audioEnabled}
                    onValueChange={(v) => {
                        setAudioEnabled(v);
                        announce(v ? 'Audio on' : 'Audio off');
                    }}
                    trackColor={{ true: '#4a90e2' }}
                />
            </SettingRow>

            <SettingRow
                label="Vibration Alerts"
                description="Temple motor feedback"
            >
                <Switch
                    value={vibrationEnabled}
                    onValueChange={(v) => {
                        setVibrationEnabled(v);
                        announce(v ? 'Vibration on' : 'Vibration off');
                    }}
                    trackColor={{ true: '#4a90e2' }}
                />
            </SettingRow>


            {/* Section: Sensitivity */}
            <Text style={styles.section}>Sensitivity</Text>

            <View style={styles.sliderRow}>
                <Text style={styles.sliderLabel}>
                    Detection Range: {sensitivity.toFixed(1)}x
                </Text>
                <Text style={styles.sliderDesc}>
                    Higher = alerts from further away
                </Text>
                <Slider
                    style={styles.slider}
                    minimumValue={0.5}
                    maximumValue={2.0}
                    step={0.1}
                    value={sensitivity}
                    onValueChange={setSensitivity}
                    onSlidingComplete={(v) =>
                        announce(`Sensitivity set to ${v.toFixed(1)}`)
                    }
                    minimumTrackTintColor="#4a90e2"
                    thumbTintColor="#4a90e2"
                />
            </View>

            <View style={styles.sliderRow}>
                <Text style={styles.sliderLabel}>
                    Vibration Intensity: {Math.round(vibIntensity * 100)}%
                </Text>
                <Slider
                    style={styles.slider}
                    minimumValue={0}
                    maximumValue={1}
                    step={0.05}
                    value={vibIntensity}
                    onValueChange={setVibIntensity}
                    minimumTrackTintColor="#4a90e2"
                    thumbTintColor="#4a90e2"
                />
            </View>

            <View style={styles.sliderRow}>
                <Text style={styles.sliderLabel}>
                    Audio Volume: {Math.round(audioVolume * 100)}%
                </Text>
                <Slider
                    style={styles.slider}
                    minimumValue={0}
                    maximumValue={1}
                    step={0.05}
                    value={audioVolume}
                    onValueChange={setAudioVolume}
                    minimumTrackTintColor="#4a90e2"
                    thumbTintColor="#4a90e2"
                />
            </View>


            {/* Section: Language */}
            <Text style={styles.section}>Language</Text>

            <View style={styles.languageRow}>
                {LANGUAGES.map(lang => (
                    <TouchableOpacity
                        key={lang}
                        style={[
                            styles.langButton,
                            language === lang && styles.langButtonActive
                        ]}
                        onPress={() => {
                            setLanguage(lang);
                            Tts.setDefaultLanguage(lang);
                            Tts.speak('Language changed');
                        }}
                    >
                        <Text style={[
                            styles.langText,
                            language === lang && styles.langTextActive
                        ]}>
                            {lang}
                        </Text>
                    </TouchableOpacity>
                ))}
            </View>

            {/* Bottom padding */}
            <View style={{ height: 40 }} />

        </ScrollView>
    );
}


// ── Styles ────────────────────────────────
const styles = StyleSheet.create({
    container: {
        flex: 1,
        backgroundColor: '#1a1a2e',
        padding: 16,
    },
    section: {
        color: '#4a90e2',
        fontSize: 13,
        fontWeight: 'bold',
        letterSpacing: 2,
        textTransform: 'uppercase',
        marginTop: 24,
        marginBottom: 8,
    },
    settingRow: {
        flexDirection: 'row',
        justifyContent: 'space-between',
        alignItems: 'center',
        backgroundColor: '#2d2d4e',
        borderRadius: 10,
        padding: 14,
        marginBottom: 8,
    },
    settingInfo: {
        flex: 1,
        marginRight: 12,
    },
    settingLabel: {
        color: '#ffffff',
        fontSize: 15,
        fontWeight: '600',
    },
    settingDesc: {
        color: '#8888aa',
        fontSize: 12,
        marginTop: 2,
    },
    sliderRow: {
        backgroundColor: '#2d2d4e',
        borderRadius: 10,
        padding: 14,
        marginBottom: 8,
    },
    sliderLabel: {
        color: '#ffffff',
        fontSize: 14,
        fontWeight: '600',
        marginBottom: 2,
    },
    sliderDesc: {
        color: '#8888aa',
        fontSize: 12,
        marginBottom: 8,
    },
    slider: {
        width: '100%',
        height: 36,
    },
    languageRow: {
        flexDirection: 'row',
        flexWrap: 'wrap',
        gap: 8,
    },
    langButton: {
        backgroundColor: '#2d2d4e',
        paddingHorizontal: 14,
        paddingVertical: 8,
        borderRadius: 8,
        borderWidth: 1,
        borderColor: '#3d3d6e',
    },
    langButtonActive: {
        borderColor: '#4a90e2',
        backgroundColor: '#1a2d4e',
    },
    langText: {
        color: '#8888aa',
        fontSize: 13,
    },
    langTextActive: {
        color: '#4a90e2',
        fontWeight: 'bold',
    },
});