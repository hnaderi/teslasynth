/**
 * Copyright Hossein Naderi 2025, 2026
 * SPDX-License-Identifier: GPL-3.0-only
 */

import { render } from 'preact';
import App from './App.jsx';
import '@teslasynth/ui/styles/style';
import '@xterm/xterm/css/xterm.css';

render(<App />, document.getElementById('app'));
