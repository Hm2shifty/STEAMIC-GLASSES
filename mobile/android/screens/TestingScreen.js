// ─────────────────────────────────────────
//  EchoNav — android/screens/TestingScreen.js
//  Used during the user testing sessions
//  described in paper §3.2 step 8.
//
//  Lets the tester:
//   - Start a new session (1, 2, or 3)
//   - Log heart rate before and during
//   - Mark task completion (3 tasks)
//   - Add notes
//   - Export results as CSV
// ─────────────────────────────────────────

import React, { useState } from 'react';
import {
    View, Text, TouchableOpacity,
    StyleSheet, ScrollView, TextInput, Alert
} from 'react-native';

import BLEManager from '../bluetooth/BLEManager';


// The 3 tasks from paper §3.2
const TASKS = [
    { id: 'hallway_clear',   label: 'Walk 15m hallway (2 obstacles)' },
    { id: 'door_found',      label: 'Locate and reach door'          },
    { id: 'person_detected', label: 'Detect approaching person'      },
];


export default function TestingScreen() {

    const [sessionId,    setSessionId]    = useState(1);
    const [bpmBefore,    setBpmBefore]    = useState('');
    const [bpmDuring,    setBpmDuring]    = useState('');
    const [tasksDone,    setTasksDone]    = useState({});
    const [notes,        setNotes]        = useState('');
    const [sessionActive,setSessionActive]= useState(false);
    const [saved,        setSaved]        = useState(false);


    // ── Toggle task completion ────────────
    const toggleTask = (taskId) => {
        setTasksDone(prev => ({
            ...prev,
            [taskId]: !prev[taskId]
        }));
    };


    // ── Start session ─────────────────────
    const startSession = () => {
        setSessionActive(true);
        setSaved(false);
        setTasksDone({});
        setBpmBefore('');
        setBpmDuring('');
        setNotes('');
    };


    // ── Send BPM to ESP32 ─────────────────
    const sendBPM = (bpm) => {
        const parsed = parseInt(bpm);
        if (isNaN(parsed) || parsed < 30 || parsed > 220) {
            Alert.alert('Invalid BPM', 'Enter a value between 30 and 220');
            return;
        }
        BLEManager.sendHeartRate(parsed, sessionId);
        Alert.alert('Sent', `BPM ${parsed} sent to glasses`);
    };


    // ── Save session results ──────────────
    // In a real app this would write to a CSV file.
    // For the prototype it just shows a summary.
    const saveSession = () => {
        const completedCount = Object.values(tasksDone).filter(Boolean).length;

        const summary = [
            `Session: ${sessionId}`,
            `BPM before: ${bpmBefore || 'not recorded'}`,
            `BPM during: ${bpmDuring || 'not recorded'}`,
            `Tasks completed: ${completedCount}/${TASKS.length}`,
            TASKS.map(t => `  ${tasksDone[t.id] ? '✓' : '✗'} ${t.label}`).join('\n'),
            notes ? `Notes: ${notes}` : ''
        ].join('\n');

        Alert.alert('Session Saved', summary);
        setSaved(true);
        setSessionActive(false);
    };


    // ── Render ────────────────────────────
    return (
        <ScrollView style={styles.container}>

            <Text style={styles.title}>Testing Session</Text>

            {/* Session selector */}
            <Text style={styles.section}>Participant</Text>
            <View style={styles.sessionRow}>
                {[1, 2, 3].map(n => (
                    <TouchableOpacity
                        key={n}
                        style={[
                            styles.sessionBtn,
                            sessionId === n && styles.sessionBtnActive
                        ]}
                        onPress={() => setSessionId(n)}
                    >
                        <Text style={[
                            styles.sessionBtnText,
                            sessionId === n && styles.sessionBtnTextActive
                        ]}>
                            P{n}
                        </Text>
                    </TouchableOpacity>
                ))}
            </View>

            {/* Start button */}
            {!sessionActive && (
                <TouchableOpacity
                    style={styles.startButton}
                    onPress={startSession}
                >
                    <Text style={styles.startButtonText}>
                        Start Session {sessionId}
                    </Text>
                </TouchableOpacity>
            )}

            {/* Session content */}
            {sessionActive && (
                <View>

                    {/* Heart rate */}
                    <Text style={styles.section}>Heart Rate (BPM)</Text>

                    <View style={styles.bpmRow}>
                        <View style={styles.bpmInput}>
                            <Text style={styles.bpmLabel}>Before</Text>
                            <TextInput
                                style={styles.input}
                                keyboardType="numeric"
                                placeholder="e.g. 72"
                                placeholderTextColor="#555577"
                                value={bpmBefore}
                                onChangeText={setBpmBefore}
                                maxLength={3}
                            />
                            <TouchableOpacity
                                style={styles.sendBtn}
                                onPress={() => sendBPM(bpmBefore)}
                            >
                                <Text style={styles.sendBtnText}>Send</Text>
                            </TouchableOpacity>
                        </View>

                        <View style={styles.bpmInput}>
                            <Text style={styles.bpmLabel}>During</Text>
                            <TextInput
                                style={styles.input}
                                keyboardType="numeric"
                                placeholder="e.g. 68"
                                placeholderTextColor="#555577"
                                value={bpmDuring}
                                onChangeText={setBpmDuring}
                                maxLength={3}
                            />
                            <TouchableOpacity
                                style={styles.sendBtn}
                                onPress={() => sendBPM(bpmDuring)}
                            >
                                <Text style={styles.sendBtnText}>Send</Text>
                            </TouchableOpacity>
                        </View>
                    </View>


                    {/* Tasks */}
                    <Text style={styles.section}>Task Completion</Text>

                    {TASKS.map(task => (
                        <TouchableOpacity
                            key={task.id}
                            style={[
                                styles.taskRow,
                                tasksDone[task.id] && styles.taskRowDone
                            ]}
                            onPress={() => toggleTask(task.id)}
                        >
                            <Text style={styles.taskCheck}>
                                {tasksDone[task.id] ? '✓' : '○'}
                            </Text>
                            <Text style={[
                                styles.taskLabel,
                                tasksDone[task.id] && styles.taskLabelDone
                            ]}>
                                {task.label}
                            </Text>
                        </TouchableOpacity>
                    ))}


                    {/* Notes */}
                    <Text style={styles.section}>Notes</Text>
                    <TextInput
                        style={styles.notesInput}
                        multiline
                        placeholder="Participant feedback, observations..."
                        placeholderTextColor="#555577"
                        value={notes}
                        onChangeText={setNotes}
                        numberOfLines={4}
                    />


                    {/* Save */}
                    <TouchableOpacity
                        style={styles.saveButton}
                        onPress={saveSession}
                    >
                        <Text style={styles.saveButtonText}>
                            Save Session Results
                        </Text>
                    </TouchableOpacity>

                </View>
            )}

            {saved && (
                <Text style={styles.savedText}>
                    ✓ Session {sessionId} saved
                </Text>
            )}

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
    title: {
        color: '#ffffff',
        fontSize: 22,
        fontWeight: 'bold',
        marginBottom: 8,
    },
    section: {
        color: '#4a90e2',
        fontSize: 12,
        fontWeight: 'bold',
        letterSpacing: 2,
        textTransform: 'uppercase',
        marginTop: 20,
        marginBottom: 8,
    },
    sessionRow: {
        flexDirection: 'row',
        gap: 10,
    },
    sessionBtn: {
        flex: 1,
        backgroundColor: '#2d2d4e',
        paddingVertical: 12,
        borderRadius: 8,
        alignItems: 'center',
        borderWidth: 1,
        borderColor: '#3d3d6e',
    },
    sessionBtnActive: {
        borderColor: '#4a90e2',
        backgroundColor: '#1a2d4e',
    },
    sessionBtnText: {
        color: '#8888aa',
        fontWeight: 'bold',
        fontSize: 16,
    },
    sessionBtnTextActive: {
        color: '#4a90e2',
    },
    startButton: {
        backgroundColor: '#4a90e2',
        paddingVertical: 16,
        borderRadius: 12,
        alignItems: 'center',
        marginTop: 20,
    },
    startButtonText: {
        color: '#ffffff',
        fontSize: 16,
        fontWeight: 'bold',
    },
    bpmRow: {
        flexDirection: 'row',
        gap: 10,
    },
    bpmInput: {
        flex: 1,
        backgroundColor: '#2d2d4e',
        borderRadius: 10,
        padding: 12,
    },
    bpmLabel: {
        color: '#8888aa',
        fontSize: 12,
        marginBottom: 6,
    },
    input: {
        backgroundColor: '#1a1a2e',
        color: '#ffffff',
        borderRadius: 8,
        padding: 10,
        fontSize: 20,
        fontWeight: 'bold',
        textAlign: 'center',
        marginBottom: 8,
    },
    sendBtn: {
        backgroundColor: '#4a90e2',
        borderRadius: 6,
        paddingVertical: 6,
        alignItems: 'center',
    },
    sendBtnText: {
        color: '#ffffff',
        fontSize: 13,
        fontWeight: 'bold',
    },
    taskRow: {
        flexDirection: 'row',
        alignItems: 'center',
        backgroundColor: '#2d2d4e',
        borderRadius: 10,
        padding: 14,
        marginBottom: 8,
        gap: 12,
    },
    taskRowDone: {
        backgroundColor: '#1a3d2e',
    },
    taskCheck: {
        color: '#2ecc71',
        fontSize: 20,
        width: 24,
    },
    taskLabel: {
        color: '#aaaacc',
        fontSize: 14,
        flex: 1,
    },
    taskLabelDone: {
        color: '#2ecc71',
    },
    notesInput: {
        backgroundColor: '#2d2d4e',
        color: '#ffffff',
        borderRadius: 10,
        padding: 12,
        fontSize: 14,
        textAlignVertical: 'top',
        minHeight: 100,
    },
    saveButton: {
        backgroundColor: '#2ecc71',
        paddingVertical: 16,
        borderRadius: 12,
        alignItems: 'center',
        marginTop: 20,
    },
    saveButtonText: {
        color: '#ffffff',
        fontSize: 16,
        fontWeight: 'bold',
    },
    savedText: {
        color: '#2ecc71',
        textAlign: 'center',
        fontSize: 15,
        marginTop: 16,
    },
});