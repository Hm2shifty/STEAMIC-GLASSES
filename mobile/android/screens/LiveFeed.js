// ─────────────────────────────────────────
//  EchoNav — android/screens/LiveFeed.js
//  Shows a live scrolling list of every
//  detection event coming from the glasses.
//  Each row shows:
//   - Object label (person, car, door etc)
//   - Distance in metres
//   - Direction (LEFT / CENTER / RIGHT)
//   - Priority color (RED / YELLOW / GREEN)
//   - Timestamp
// ─────────────────────────────────────────

import React, { useState, useEffect, useRef } from 'react';
import {
    View, Text, FlatList,
    StyleSheet, TouchableOpacity
} from 'react-native';

import BLEManager, {
    PRIORITY_LABELS,
    PRIORITY_COLORS,
    DIR_LABELS
} from '../bluetooth/BLEManager';


export default function LiveFeed() {

    const [events,   setEvents]   = useState([]);
    const [paused,   setPaused]   = useState(false);
    const listRef = useRef(null);


    // ── Subscribe to BLE detections ───────
    useEffect(() => {
        BLEManager.onDetection = (packet) => {
            if (paused) return;

            setEvents(prev => {
                // Keep last 50 events so list doesn't grow forever
                const updated = [packet, ...prev].slice(0, 50);
                return updated;
            });
        };

        return () => {
            BLEManager.onDetection = null;
        };
    }, [paused]);


    // ── Render one detection row ──────────
    const renderEvent = ({ item }) => (
        <View style={[
            styles.eventRow,
            { borderLeftColor: item.color, borderLeftWidth: 4 }
        ]}>
            <View style={styles.eventLeft}>
                <Text style={[styles.eventLabel, { color: item.color }]}>
                    {item.label.toUpperCase()}
                </Text>
                <Text style={styles.eventDetail}>
                    {item.distance}m away · {DIR_LABELS[item.direction]}
                </Text>
            </View>
            <View style={styles.eventRight}>
                <Text style={[styles.priorityBadge, { color: item.color }]}>
                    {PRIORITY_LABELS[item.priority]}
                </Text>
                <Text style={styles.eventTime}>{item.timestamp}</Text>
            </View>
        </View>
    );


    // ── Render ────────────────────────────
    return (
        <View style={styles.container}>

            {/* Header row */}
            <View style={styles.header}>
                <Text style={styles.headerText}>
                    {events.length} events
                </Text>
                <View style={styles.headerButtons}>
                    <TouchableOpacity
                        style={[styles.headerBtn,
                            paused && styles.headerBtnActive]}
                        onPress={() => setPaused(p => !p)}
                    >
                        <Text style={styles.headerBtnText}>
                            {paused ? '▶ Resume' : '⏸ Pause'}
                        </Text>
                    </TouchableOpacity>

                    <TouchableOpacity
                        style={styles.headerBtn}
                        onPress={() => setEvents([])}
                    >
                        <Text style={styles.headerBtnText}>Clear</Text>
                    </TouchableOpacity>
                </View>
            </View>

            {/* Legend */}
            <View style={styles.legend}>
                {[
                    { label: 'HIGH  < 1m',    color: '#e74c3c' },
                    { label: 'MED  1–3m',     color: '#f39c12' },
                    { label: 'LOW  > 3m',     color: '#2ecc71' },
                ].map(item => (
                    <View key={item.label} style={styles.legendItem}>
                        <View style={[
                            styles.legendDot,
                            { backgroundColor: item.color }
                        ]} />
                        <Text style={styles.legendText}>{item.label}</Text>
                    </View>
                ))}
            </View>

            {/* Events list */}
            {events.length === 0 ? (
                <View style={styles.empty}>
                    <Text style={styles.emptyText}>
                        {BLEManager.isConnected()
                            ? 'Waiting for detections...'
                            : 'Connect to glasses on Home screen first'}
                    </Text>
                </View>
            ) : (
                <FlatList
                    ref={listRef}
                    data={events}
                    keyExtractor={(_, i) => i.toString()}
                    renderItem={renderEvent}
                    style={styles.list}
                />
            )}

        </View>
    );
}


// ── Styles ────────────────────────────────
const styles = StyleSheet.create({
    container: {
        flex: 1,
        backgroundColor: '#1a1a2e',
    },
    header: {
        flexDirection: 'row',
        justifyContent: 'space-between',
        alignItems: 'center',
        padding: 12,
        borderBottomWidth: 1,
        borderBottomColor: '#2d2d4e',
    },
    headerText: {
        color: '#8888aa',
        fontSize: 13,
    },
    headerButtons: {
        flexDirection: 'row',
        gap: 8,
    },
    headerBtn: {
        backgroundColor: '#2d2d4e',
        paddingHorizontal: 14,
        paddingVertical: 6,
        borderRadius: 8,
    },
    headerBtnActive: {
        backgroundColor: '#4a2d2d',
    },
    headerBtnText: {
        color: '#aaaacc',
        fontSize: 13,
    },
    legend: {
        flexDirection: 'row',
        justifyContent: 'space-around',
        padding: 10,
        backgroundColor: '#16162a',
    },
    legendItem: {
        flexDirection: 'row',
        alignItems: 'center',
        gap: 6,
    },
    legendDot: {
        width: 10,
        height: 10,
        borderRadius: 5,
    },
    legendText: {
        color: '#8888aa',
        fontSize: 12,
    },
    list: {
        flex: 1,
        padding: 8,
    },
    eventRow: {
        flexDirection: 'row',
        justifyContent: 'space-between',
        backgroundColor: '#2d2d4e',
        borderRadius: 8,
        padding: 12,
        marginBottom: 6,
    },
    eventLeft: {
        flex: 1,
    },
    eventLabel: {
        fontWeight: 'bold',
        fontSize: 15,
        marginBottom: 3,
    },
    eventDetail: {
        color: '#aaaacc',
        fontSize: 13,
    },
    eventRight: {
        alignItems: 'flex-end',
    },
    priorityBadge: {
        fontWeight: 'bold',
        fontSize: 13,
        marginBottom: 4,
    },
    eventTime: {
        color: '#666688',
        fontSize: 11,
    },
    empty: {
        flex: 1,
        alignItems: 'center',
        justifyContent: 'center',
    },
    emptyText: {
        color: '#8888aa',
        fontSize: 15,
        textAlign: 'center',
        paddingHorizontal: 40,
    },
});