/*
 * SynDimm - Auto-generated PROGMEM file
 * Source: style.css
 */

#ifndef CSS_STYLE_H
#define CSS_STYLE_H

const char CSS_STYLE[] PROGMEM = R"=====(
/* ===================================
   SynDimm - Minimal Control Panel
   Inspired by smartkraft.ch design
   =================================== */

/* ========== Reset ========== */
* {
    margin: 0;
    padding: 0;
    box-sizing: border-box;
}

/* ========== Variables ========== */
:root {
    --bg: #000;
    --bg-raised: #0d0d0d;
    --bg-panel: #121212;
    --border: #1f1f1f;
    --border-hover: #2a2a2a;
    --text: #fff;
    --text-secondary: #999;
    --text-muted: #666;
    --spacing-xs: 8px;
    --spacing-sm: 12px;
    --spacing-md: 16px;
    --spacing-lg: 24px;
    --spacing-xl: 32px;
    --radius: 6px;
    --transition: 200ms ease;
}

/* Light Theme */
[data-theme="light"] {
    --bg: #ffffff;
    --bg-raised: #f8f8f8;
    --bg-panel: #f0f0f0;
    --border: #e0e0e0;
    --border-hover: #d0d0d0;
    --text: #000;
    --text-secondary: #666;
    --text-muted: #999;
}

/* ========== Base ========== */
body {
    background: var(--bg);
    color: var(--text);
    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', 'Roboto', 'Oxygen', 'Ubuntu', 'Helvetica Neue', Arial, sans-serif;
    line-height: 1.6;
    -webkit-font-smoothing: antialiased;
}

.container {
    max-width: 780px;
    margin: 0 auto;
    padding: 0 var(--spacing-md);
}

/* ========== Header ========== */
.header {
    border-bottom: 1px solid var(--border);
    padding: var(--spacing-md) 0 var(--spacing-sm);
}

.header-content {
    display: flex;
    justify-content: center;
    align-items: center;
}

.brand {
    text-align: center;
}

.brand h1 {
    font-size: 2.45rem;
    font-weight: 600;
    letter-spacing: 0.5px;
    margin-bottom: 4px;
}

.brand-info {
    display: flex;
    gap: 16px;
    justify-content: center;
    align-items: center;
    font-size: 0.9rem;
    color: var(--text-secondary);
    margin-top: 8px;
}

.chip-id, .version-info {
    display: inline-flex;
    align-items: center;
    gap: 4px;
}

.brand-link {
    font-size: 1.05rem;
    color: var(--text-secondary);
    letter-spacing: 1px;
    text-transform: uppercase;
    text-decoration: none;
    transition: color var(--transition);
}

.brand-link:hover {
    color: var(--text);
}

/* ========== Navigation ========== */
.nav {
    border-bottom: 1px solid var(--border);
    padding: var(--spacing-lg) 0 0;
}

.nav-links {
    display: flex;
    gap: 0;
}

.nav-link {
    flex: 1;
    padding: var(--spacing-sm) var(--spacing-md);
    color: var(--text-secondary);
    text-decoration: none;
    text-align: center;
    border-bottom: 2px solid transparent;
    transition: all var(--transition);
    font-size: 0.9rem;
    font-weight: 500;
}

.nav-link:hover {
    color: var(--text);
    background: var(--bg-raised);
}

.nav-link.active {
    color: var(--text);
    border-bottom-color: var(--text);
}

/* ========== Main Content ========== */
.main {
    padding: var(--spacing-xl) 0;
}

.tab-section {
    display: none;
}

.tab-section.active {
    display: block;
    animation: fadeIn var(--transition);
}

@keyframes fadeIn {
    from { opacity: 0; }
    to { opacity: 1; }
}

/* ========== Panel ========== */
.panel {
    background: var(--bg-panel);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    padding: var(--spacing-lg);
    margin-bottom: var(--spacing-md);
}

.panel.center {
    text-align: center;
}

.panel-title {
    font-size: 1rem;
    font-weight: 600;
    margin-bottom: var(--spacing-md);
}

.panel-text {
    color: var(--text-secondary);
    line-height: 1.7;
    font-size: 0.95rem;
    padding: var(--spacing-md) var(--spacing-lg);
}

.panel-text.muted {
    color: var(--text-muted);
    margin-bottom: var(--spacing-sm);
}

/* ========== Section Divider ========== */
.section-divider {
    height: 1px;
    background: var(--border);
    margin: var(--spacing-lg) 0;
}

/* ========== Section Header ========== */
.section-header {
    text-align: center;
    font-size: 0.95rem;
    font-weight: 600;
    color: var(--text);
    margin-bottom: var(--spacing-lg);
    letter-spacing: 0.5px;
}

/* ========== Accordion ========== */
.accordion-panel {
    background: var(--bg-panel);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    margin-bottom: var(--spacing-md);
    overflow: hidden;
}

.accordion-header {
    display: flex;
    align-items: center;
    gap: var(--spacing-md);
    padding: var(--spacing-md) var(--spacing-lg);
    cursor: pointer;
    transition: background var(--transition);
    user-select: none;
}

.accordion-header:hover {
    background: var(--bg-raised);
}

.accordion-title {
    flex: 1;
    font-size: 0.875rem;
    font-weight: 700;
    letter-spacing: 0.05em;
}

.accordion-status {
    font-size: 0.75rem;
    padding: 4px 12px;
    border-radius: 3px;
    font-weight: 600;
    text-transform: capitalize;
}

.accordion-status.connected {
    background: #00d4aa;
    color: #000;
}

.accordion-status.inactive {
    background: transparent;
    color: var(--text-secondary);
    border: 1px solid var(--border);
}

.accordion-status.not-configured {
    background: transparent;
    color: var(--text-secondary);
    border: 1px solid var(--border);
}

.accordion-arrow {
    font-size: 0.75rem;
    color: var(--text-secondary);
    transition: transform var(--transition);
}

.accordion-header.active .accordion-arrow {
    transform: rotate(180deg);
}

.accordion-content {
    max-height: 0;
    overflow: hidden;
    transition: max-height 300ms ease;
}

.accordion-content:not(.collapsed) {
    max-height: 1000px;
}

/* ========== Info Row ========== */
.info-row {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: var(--spacing-md) var(--spacing-lg);
    border-top: 1px solid var(--border);
}

.info-row:first-child {
    border-top: none;
    padding-top: var(--spacing-md);
}

.info-row:last-child {
    padding-bottom: var(--spacing-md);
}

.info-label {
    font-size: 0.875rem;
    color: var(--text-secondary);
    font-weight: 500;
}

.info-value {
    font-size: 0.875rem;
    color: var(--text);
    font-weight: 600;
}

/* ========== Network Save ========== */
.network-save {
    padding: var(--spacing-xl) var(--spacing-lg);
    text-align: center;
}

.btn-save {
    padding: var(--spacing-md) var(--spacing-xl);
    background: var(--bg);
    border: 2px solid var(--text);
    border-radius: var(--radius);
    color: var(--text);
    font-size: 0.95rem;
    font-weight: 600;
    cursor: pointer;
    transition: all 0.2s;
    min-width: 250px;
}

.btn-save:hover {
    background: var(--text);
    color: var(--bg);
}

/* ========== Mode Grid ========== */
.mode-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
    gap: var(--spacing-md);
}

.mode-card {
    position: relative;
    display: block;
    background: var(--bg);
    border: 2px solid var(--border);
    border-radius: var(--radius);
    padding: var(--spacing-md);
    cursor: pointer;
    transition: all var(--transition);
}

.mode-card:hover {
    border-color: var(--border-hover);
}

.mode-card.active {
    border-color: var(--text);
}

.mode-card input {
    position: absolute;
    opacity: 0;
    pointer-events: none;
}

.mode-content {
    display: flex;
    flex-direction: column;
    gap: 4px;
}

.mode-name {
    font-size: 1rem;
    font-weight: 600;
}

.mode-desc {
    font-size: 0.85rem;
    color: var(--text-secondary);
}

.mode-check {
    position: absolute;
    top: var(--spacing-sm);
    right: var(--spacing-sm);
    width: 18px;
    height: 18px;
    border: 2px solid var(--border);
    border-radius: 50%;
    transition: all var(--transition);
}

.mode-card.active .mode-check {
    border-color: var(--text);
    background: var(--text);
}

.mode-card.active .mode-check::after {
    content: '';
    position: absolute;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    width: 6px;
    height: 6px;
    background: var(--bg);
    border-radius: 50%;
}

/* ========== Range Slider ========== */
.range-slider {
    -webkit-appearance: none;
    appearance: none;
    width: 100%;
    height: 6px;
    background: var(--bg);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    outline: none;
    cursor: pointer;
    position: relative;
}

.range-slider::-webkit-slider-thumb {
    -webkit-appearance: none;
    width: 20px;
    height: 20px;
    background: var(--text);
    border: 2px solid var(--bg);
    border-radius: 50%;
    cursor: pointer;
    transition: transform var(--transition);
}

.range-slider::-webkit-slider-thumb:hover {
    transform: scale(1.1);
}

.range-slider::-moz-range-thumb {
    width: 20px;
    height: 20px;
    background: var(--text);
    border: 2px solid var(--bg);
    border-radius: 50%;
    cursor: pointer;
    transition: transform var(--transition);
}

.range-slider::-moz-range-thumb:hover {
    transform: scale(1.1);
}

.slider-value {
    display: block;
    text-align: center;
    font-size: 1.5rem;
    font-weight: 700;
    color: var(--text);
    margin-bottom: var(--spacing-xs);
}

.slider-controls {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: var(--spacing-lg);
    padding: var(--spacing-md) var(--spacing-lg);
    margin-bottom: var(--spacing-sm);
}

.slider-group {
    display: flex;
    flex-direction: column;
}

.slider-label {
    font-size: 0.85rem;
    font-weight: 600;
    color: var(--text);
    text-align: center;
    margin-bottom: var(--spacing-xs);
}

/* ========== Device Connection ========== */
.device-connection {
    background: var(--bg);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    padding: var(--spacing-md) var(--spacing-lg);
    margin-bottom: var(--spacing-md);
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: var(--spacing-md);
}

.device-connection.connected {
    border-color: #00d4aa;
    background: rgba(0, 212, 170, 0.05);
}

.device-info-left {
    display: flex;
    align-items: center;
    gap: var(--spacing-sm);
    flex: 1;
}

.device-name {
    font-size: 0.95rem;
    font-weight: 600;
    color: var(--text);
}

.device-ip {
    font-size: 0.85rem;
    color: var(--text-secondary);
    margin-left: var(--spacing-xs);
}

.device-toggle {
    position: relative;
    display: inline-block;
    width: 52px;
    height: 28px;
}

.device-toggle input {
    opacity: 0;
    width: 0;
    height: 0;
}

.device-toggle-slider {
    position: absolute;
    cursor: pointer;
    inset: 0;
    background: var(--bg);
    border: 2px solid var(--border);
    border-radius: 28px;
    transition: var(--transition);
}

.device-toggle-slider::before {
    content: '';
    position: absolute;
    height: 18px;
    width: 18px;
    left: 3px;
    bottom: 3px;
    background: var(--text-muted);
    border-radius: 50%;
    transition: var(--transition);
}

.device-toggle input:checked + .device-toggle-slider {
    border-color: #00d4aa;
    background: rgba(0, 212, 170, 0.15);
}

.device-toggle input:checked + .device-toggle-slider::before {
    transform: translateX(24px);
    background: #00d4aa;
}

/* ========== Preferences ========== */
.preferences-row {
    display: flex;
    align-items: center;
    gap: var(--spacing-lg);
}

.pref-section {
    flex: 1;
}

.pref-label {
    display: block;
    font-size: 0.85rem;
    font-weight: 500;
    margin-bottom: var(--spacing-sm);
    color: var(--text-secondary);
}

.pref-divider {
    width: 1px;
    height: 60px;
    background: var(--border);
}

.option-group {
    display: flex;
    gap: var(--spacing-xs);
}

.option-group.compact {
    gap: 6px;
}

.option-group.compact .option-card {
    padding: 8px 12px;
    font-size: 0.85rem;
}

.option-card {
    flex: 1;
    display: block;
    padding: var(--spacing-sm);
    background: var(--bg);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    text-align: center;
    cursor: pointer;
    transition: all var(--transition);
    font-size: 0.9rem;
    color: var(--text-secondary);
}

.option-card:hover {
    border-color: var(--border-hover);
    color: var(--text);
}

.option-card.active {
    background: var(--text);
    color: var(--bg);
    border-color: var(--text);
}

.option-card input {
    position: absolute;
    opacity: 0;
    pointer-events: none;
}

/* ========== Form Fields ========== */
.form-field {
    margin: 0 var(--spacing-lg) var(--spacing-md);
}

.form-field:first-child {
    margin-top: var(--spacing-md);
}

.field-label {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: var(--spacing-sm);
    font-size: 0.9rem;
    font-weight: 500;
}

.field-hint {
    font-size: 0.75rem;
    color: var(--text-muted);
    font-weight: 400;
}

/* ========== Buttons ========== */
.btn {
    display: inline-block;
    padding: var(--spacing-sm) var(--spacing-lg);
    background: var(--text);
    color: var(--bg);
    border: none;
    border-radius: var(--radius);
    font-size: 0.95rem;
    font-weight: 600;
    font-family: inherit;
    cursor: pointer;
    transition: all var(--transition);
}

.btn:hover {
    opacity: 0.9;
    transform: translateY(-1px);
}

.btn:active {
    transform: translateY(0);
}

.btn-full {
    width: 100%;
}

.btn-scan {
    padding: var(--spacing-sm) var(--spacing-xl);
    min-width: 200px;
}

.scan-section {
    text-align: center;
    padding: var(--spacing-md) var(--spacing-lg);
}

/* ========== Status Dot ========== */
.status-dot {
    width: 6px;
    height: 6px;
    border-radius: 50%;
    background: var(--text);
    animation: pulse 2s infinite;
}

.status-dot.inactive {
    background: var(--text-muted);
    animation: none;
}

@keyframes pulse {
    0%, 100% { opacity: 1; }
    50% { opacity: 0.4; }
}

/* ========== Device List ========== */
.device-list {
    margin: var(--spacing-md) var(--spacing-lg);
}

.empty-state {
    text-align: center;
    padding: var(--spacing-lg);
    background: var(--bg);
    border: 1px solid var(--border);
    border-radius: var(--radius);
}

.empty-state p {
    font-size: 0.95rem;
    margin-bottom: 4px;
}

.empty-state span {
    font-size: 0.85rem;
    color: var(--text-muted);
}

.device-item {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: var(--spacing-md);
    background: var(--bg);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    margin-bottom: var(--spacing-xs);
    transition: all var(--transition);
}

.device-item:hover {
    border-color: var(--border-hover);
}

.device-item:last-child {
    margin-bottom: 0;
}

.device-info {
    display: flex;
    flex-direction: column;
    gap: 4px;
    flex: 1;
}

.device-name {
    font-size: 0.95rem;
    font-weight: 600;
    color: var(--text);
}

.device-detail {
    font-size: 0.8rem;
    color: var(--text-secondary);
}

.device-action {
    margin-left: var(--spacing-md);
}

.btn-connect {
    padding: 8px 20px;
    background: var(--text);
    color: var(--bg);
    border: none;
    border-radius: var(--radius);
    font-size: 0.85rem;
    font-weight: 600;
    cursor: pointer;
    transition: all var(--transition);
}

.btn-connect:hover {
    opacity: 0.9;
    transform: translateY(-1px);
}

.btn-connect:active {
    transform: translateY(0);
}

.btn-disconnect {
    padding: 8px 20px;
    background: #dc2626;
    color: #fff;
    border: none;
    border-radius: var(--radius);
    font-size: 0.85rem;
    font-weight: 600;
    cursor: pointer;
    transition: all var(--transition);
}

.btn-disconnect:hover {
    background: #b91c1c;
}

/* ========== Network Status ========== */
.network-status {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 1px;
    background: var(--border);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    overflow: hidden;
    margin-bottom: var(--spacing-lg);
}

.dimmer-status {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 1px;
    background: var(--border);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    overflow: hidden;
    margin: var(--spacing-md) var(--spacing-lg) var(--spacing-lg);
}

.status-item {
    display: flex;
    flex-direction: column;
    gap: var(--spacing-xs);
    padding: var(--spacing-md);
    background: var(--bg);
    align-items: center;
    text-align: center;
}

.status-item.status-toggle {
    align-items: center;
    justify-content: center;
}

.status-item.status-toggle .status-label {
    margin-bottom: var(--spacing-xs);
}

.status-label {
    font-size: 0.75rem;
    color: var(--text-secondary);
    text-transform: uppercase;
    letter-spacing: 0.05em;
    font-weight: 600;
}

.status-value {
    font-size: 1rem;
    color: var(--text);
    font-weight: 600;
}

.status-value-stacked {
    display: flex;
    flex-direction: column;
    gap: 4px;
    align-items: center;
}

.ip-value {
    font-size: 1rem;
    color: var(--text);
    font-weight: 600;
}

.domain-value {
    font-size: 0.85rem;
    color: var(--text-secondary);
    font-weight: 500;
}

/* ========== Form Group ========== */
.form-group {
    display: flex;
    flex-direction: column;
    gap: var(--spacing-xs);
    padding: 0 var(--spacing-lg);
    margin-bottom: var(--spacing-md);
}

.form-group:first-child {
    padding-top: var(--spacing-md);
}

.form-group:last-child {
    padding-bottom: var(--spacing-md);
}

.form-group label {
    font-size: 0.875rem;
    font-weight: 600;
    color: var(--text);
}

.form-group input {
    padding: var(--spacing-sm) var(--spacing-md);
    background: rgba(255, 255, 255, 0.05);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    font-size: 0.875rem;
    color: var(--text);
    outline: none;
    transition: all 0.2s;
}

.form-group input:focus {
    border-color: var(--text);
    background: rgba(255, 255, 255, 0.08);
}

.form-group input::placeholder {
    color: var(--text-secondary);
}

.form-hint {
    font-size: 0.75rem;
    color: var(--text-muted);
    font-style: italic;
    margin-top: 4px;
}

/* ========== Info Guide ========== */
.info-guide {
    padding: var(--spacing-xl);
    padding-bottom: var(--spacing-lg);
    line-height: 1.8;
}

.info-guide h3 {
    font-size: 1.25rem;
    font-weight: 600;
    color: var(--text);
    margin-bottom: var(--spacing-lg);
    padding-bottom: var(--spacing-sm);
    border-bottom: 1px solid var(--border);
}

.info-guide h4 {
    font-size: 1rem;
    font-weight: 600;
    color: var(--text);
    margin-top: var(--spacing-lg);
    margin-bottom: var(--spacing-sm);
}

.info-guide p {
    margin-bottom: var(--spacing-md);
    color: var(--text);
    font-size: 0.95rem;
}

.info-guide ul {
    margin-bottom: var(--spacing-md);
    margin-left: var(--spacing-lg);
    color: var(--text-secondary);
    font-size: 0.9rem;
}

.info-guide ul li {
    margin-bottom: var(--spacing-xs);
}

.info-guide ul li strong {
    color: var(--text);
}

.info-guide p:last-child {
    margin-bottom: 0;
}

.warning-text {
    background: rgba(255, 165, 0, 0.1);
    border-left: 3px solid #ffa500;
    padding: var(--spacing-sm) var(--spacing-md);
    margin-top: var(--spacing-md);
    color: #ffa500 !important;
    font-size: 0.875rem;
    border-radius: 4px;
}

/* ========== Info Documentation ========== */
.info-documentation {
    padding: var(--spacing-xl);
    padding-top: var(--spacing-lg);
    border-top: 1px solid var(--border);
    text-align: center;
}

.doc-title {
    font-size: 1.125rem;
    font-weight: 600;
    color: var(--text);
    margin-bottom: var(--spacing-md);
}

.doc-description {
    font-size: 0.875rem;
    color: var(--text-secondary);
    margin-bottom: var(--spacing-lg);
    line-height: 1.6;
}

.doc-buttons {
    display: flex;
    gap: 12px;
    justify-content: center;
    align-items: center;
}

.doc-button {
    display: inline-block;
    padding: 12px 32px;
    background: var(--text);
    border: 1px solid var(--text);
    border-radius: var(--radius);
    color: var(--bg);
    text-decoration: none;
    font-weight: 600;
    font-size: 0.95rem;
    transition: all 0.2s;
    text-align: center;
    min-width: 140px;
}

.doc-button:hover {
    background: var(--bg);
    color: var(--text);
    border-color: var(--text);
    transform: translateY(-2px);
    box-shadow: 0 4px 12px rgba(255, 255, 255, 0.1);
}

[data-theme="light"] .doc-button:hover {
    box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
}

/* ========== Safe Lock Styles ========== */
.safe-tabs {
    display: flex;
    gap: 4px;
    padding: var(--spacing-md) var(--spacing-lg);
    background: var(--bg);
    border-bottom: 1px solid var(--border);
    overflow-x: auto;
}

.safe-tab {
    flex: 1;
    min-width: 80px;
    padding: var(--spacing-xs) var(--spacing-sm);
    background: transparent;
    border: 1px solid var(--border);
    border-radius: var(--radius);
    color: var(--text-secondary);
    font-size: 0.8rem;
    font-weight: 600;
    cursor: pointer;
    transition: all var(--transition);
    white-space: nowrap;
}

.safe-tab:hover {
    background: var(--bg-raised);
    color: var(--text);
}

.safe-tab.active {
    background: var(--text);
    color: var(--bg);
    border-color: var(--text);
}

.safe-tab-content {
    display: none;
    padding: var(--spacing-lg);
}

.safe-tab-content.active {
    display: block;
}

.safe-section {
    background: var(--bg);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    padding: var(--spacing-md) 0;
    margin-bottom: var(--spacing-md);
}

.safe-section .section-header {
    margin-bottom: var(--spacing-md);
    padding: 0 var(--spacing-lg);
}

.toggle-switch {
    display: flex;
    align-items: center;
    gap: var(--spacing-md);
    cursor: pointer;
    user-select: none;
}

.toggle-switch input[type="checkbox"] {
    position: absolute;
    opacity: 0;
    pointer-events: none;
}

.toggle-slider {
    position: relative;
    display: inline-block;
    width: 44px;
    height: 24px;
    background: var(--bg);
    border: 1px solid var(--border);
    border-radius: 24px;
    transition: var(--transition);
    flex-shrink: 0;
}

.toggle-slider::before {
    content: '';
    position: absolute;
    height: 16px;
    width: 16px;
    left: 3px;
    bottom: 3px;
    background: var(--text-muted);
    border-radius: 50%;
    transition: var(--transition);
}

.toggle-switch input:checked + .toggle-slider {
    border-color: var(--text);
    background: var(--bg);
}

.toggle-switch input:checked + .toggle-slider::before {
    transform: translateX(20px);
    background: var(--text);
}

.toggle-label {
    font-size: 0.9rem;
    font-weight: 500;
    color: var(--text);
}

.text-input {
    width: 100%;
    padding: var(--spacing-sm) var(--spacing-md);
    background: var(--bg-raised);
    color: var(--text);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    font-size: 0.875rem;
    font-family: 'Monaco', 'Courier New', monospace;
    transition: border-color var(--transition);
}

.text-input:focus {
    outline: none;
    border-color: var(--border-hover);
    background: var(--bg);
}

.text-input::placeholder {
    color: var(--text-muted);
}

.text-area {
    width: 100%;
    padding: var(--spacing-sm) var(--spacing-md);
    background: var(--bg-raised);
    color: var(--text);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    font-size: 0.875rem;
    font-family: 'Monaco', 'Courier New', monospace;
    transition: border-color var(--transition);
    resize: vertical;
}

.text-area:focus {
    outline: none;
    border-color: var(--border-hover);
    background: var(--bg);
}

.text-area::placeholder {
    color: var(--text-muted);
}

.password-validation {
    margin-top: var(--spacing-xs);
    font-size: 0.75rem;
    padding: var(--spacing-xs) var(--spacing-sm);
    border-radius: 4px;
}

.password-validation.valid {
    color: #00d4aa;
    background: rgba(0, 212, 170, 0.1);
}

.password-validation.invalid {
    color: #ff4444;
    background: rgba(255, 68, 68, 0.1);
}

.btn-secondary {
    background: transparent;
    border: 1px solid var(--border);
    color: var(--text);
}

.btn-secondary:hover {
    background: var(--bg-raised);
    border-color: var(--text);
}

/* ========== Responsive ========== */
@media (max-width: 640px) {
    :root {
        --spacing-xl: 24px;
        --spacing-lg: 16px;
    }
    
    .header {
        padding: var(--spacing-lg) 0;
    }
    
    .brand h1 {
        font-size: 2.5rem;
    }
    
    .brand-link {
        font-size: 1.2rem;
    }
    
    .nav-links {
        flex-wrap: wrap;
    }
    
    .nav-link {
        font-size: 0.85rem;
        padding: var(--spacing-sm);
    }
    
    .mode-grid {
        grid-template-columns: 1fr;
    }
    
    .preferences-row {
        flex-direction: column;
        gap: var(--spacing-md);
    }
    
    .pref-divider {
        width: 100%;
        height: 1px;
    }
    
    .safe-tabs {
        overflow-x: auto;
        -webkit-overflow-scrolling: touch;
    }
    
    .safe-tab {
        min-width: 70px;
        font-size: 0.75rem;
    }
}
)=====";

#endif
