// ─────────────────────────────────────────
//  EchoNav — android/App.js
//  Entry point for the React Native app.
//  Sets up screen navigation between:
//   - HomeScreen    (main control panel)
//   - LiveFeed      (detection display)
//   - Settings      (user preferences)
//   - TestingScreen (heart rate logging)
//
//  Install dependencies first:
//  npm install @react-navigation/native
//  npm install @react-navigation/stack
//  npm install react-native-ble-plx
//  npm install @react-native-voice/voice
//  npm install react-native-tts
// ─────────────────────────────────────────

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createStackNavigator } from '@react-navigation/stack';
import { StatusBar } from 'react-native';

import HomeScreen    from './screens/HomeScreen';
import LiveFeed      from './screens/LiveFeed';
import SettingsScreen from './screens/SettingsScreen';
import TestingScreen from './screens/TestingScreen';

const Stack = createStackNavigator();

export default function App() {
    return (
        <NavigationContainer>
            <StatusBar barStyle="light-content" backgroundColor="#1a1a2e" />
            <Stack.Navigator
                initialRouteName="Home"
                screenOptions={{
                    headerStyle:      { backgroundColor: '#1a1a2e' },
                    headerTintColor:  '#ffffff',
                    headerTitleStyle: { fontWeight: 'bold' },
                }}
            >
                <Stack.Screen
                    name="Home"
                    component={HomeScreen}
                    options={{ title: 'EchoNav' }}
                />
                <Stack.Screen
                    name="LiveFeed"
                    component={LiveFeed}
                    options={{ title: 'Live Detection Feed' }}
                />
                <Stack.Screen
                    name="Settings"
                    component={SettingsScreen}
                    options={{ title: 'Settings' }}
                />
                <Stack.Screen
                    name="Testing"
                    component={TestingScreen}
                    options={{ title: 'Testing Session' }}
                />
            </Stack.Navigator>
        </NavigationContainer>
    );
}