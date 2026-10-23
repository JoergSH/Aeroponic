R"rawliteral(
        <!-- ============================================================ -->
        <!-- ALARME — Benachrichtigungskanäle + einzelne Alarmschwellen     -->
        <!-- ============================================================ -->
        <div id="alarmeTab" class="tab-content">

            <!-- Benachrichtigungen: WhatsApp/Telegram/E-Mail, unabhaengig voneinander aktivierbar
                 (CallMeBot/WhatsApp ist erfahrungsgemaess oefter mal down) -->
            <h3 class="section-heading">🔔 <span class="de">Benachrichtigungen</span><span class="en">Notifications</span></h3>
            <div class="status-box-info">
                <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:16px;">
                    <label style="display:flex;align-items:center;gap:8px;cursor:pointer;">
                        <span style="font-size:0.9em;"><span class="de">Benachrichtigungen aktiv (Hauptschalter)</span><span class="en">Notifications active (master switch)</span></span>
                        <input type="checkbox" id="notifyGlobalEnabled" style="width:18px;height:18px;">
                    </label>
                </div>

                <div style="font-size:0.85em;color:#555;font-weight:bold;margin-bottom:8px;"><span class="de">Kanäle</span><span class="en">Channels</span></div>
                <div style="display:flex;flex-direction:column;gap:12px;margin-bottom:20px;">
                    <!-- WhatsApp (CallMeBot) -->
                    <div style="padding:10px;background:#f8f9fa;border-radius:6px;">
                        <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:8px;">
                            <label style="display:flex;align-items:center;gap:8px;cursor:pointer;">
                                <input type="checkbox" id="notifyWhatsappEnabled" style="width:16px;height:16px;">
                                <span style="font-size:0.9em;">📱 WhatsApp (CallMeBot)</span>
                            </label>
                            <button onclick="sendNotifyTest()"
                                style="background:#25D366;color:white;border:none;padding:6px 14px;border-radius:5px;cursor:pointer;font-size:0.82em;">
                                <span class="de">Test senden</span><span class="en">Send test</span></button>
                        </div>
                        <div style="font-size:0.78em;color:#888;margin-bottom:8px;">
                            <span class="de">Kostenlos. Einmalig einrichten: dem CallMeBot-Kontakt auf WhatsApp eine Aktivierungsnachricht schicken (siehe callmebot.com/blog/free-api-whatsapp-messages), dann Telefonnummer und den per WhatsApp erhaltenen API-Key hier eintragen.</span>
                            <span class="en">Free. One-time setup: send the CallMeBot contact an activation message on WhatsApp (see callmebot.com/blog/free-api-whatsapp-messages), then enter your phone number and the API key you receive here.</span>
                        </div>
                        <div style="display:grid;grid-template-columns:1fr 1fr;gap:12px;max-width:500px;">
                            <div>
                                <div class="status-label"><span class="de">Telefonnummer</span><span class="en">Phone number</span></div>
                                <input type="text" id="notifyPhone" placeholder="+491701234567" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                            </div>
                            <div>
                                <div class="status-label">API-Key (CallMeBot)</div>
                                <input type="text" id="notifyApikey" placeholder="unverändert lassen" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                            </div>
                        </div>
                    </div>

                    <!-- Telegram -->
                    <div style="padding:10px;background:#f8f9fa;border-radius:6px;">
                        <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:8px;">
                            <label style="display:flex;align-items:center;gap:8px;cursor:pointer;">
                                <input type="checkbox" id="notifyTelegramEnabled" style="width:16px;height:16px;">
                                <span style="font-size:0.9em;">✈️ Telegram</span>
                            </label>
                            <button onclick="sendNotifyTestTelegram()"
                                style="background:#26A5E4;color:white;border:none;padding:6px 14px;border-radius:5px;cursor:pointer;font-size:0.82em;">
                                <span class="de">Test senden</span><span class="en">Send test</span></button>
                        </div>
                        <div style="font-size:0.78em;color:#888;margin-bottom:8px;">
                            <span class="de">Kostenlos. Einmalig einrichten: mit @BotFather einen Bot anlegen (liefert den Token), dann dem Bot in Telegram schreiben und die eigene Chat-ID ermitteln (z.B. über @userinfobot).</span>
                            <span class="en">Free. One-time setup: create a bot with @BotFather (gives you the token), message the bot on Telegram, and find your own chat ID (e.g. via @userinfobot).</span>
                        </div>
                        <div style="display:grid;grid-template-columns:1fr 1fr;gap:12px;max-width:500px;">
                            <div>
                                <div class="status-label">Bot-Token</div>
                                <input type="text" id="notifyTelegramToken" placeholder="unverändert lassen" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                            </div>
                            <div>
                                <div class="status-label">Chat-ID</div>
                                <input type="text" id="notifyTelegramChatId" placeholder="123456789" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                            </div>
                        </div>
                    </div>

                    <!-- E-Mail (SMTP) -->
                    <div style="padding:10px;background:#f8f9fa;border-radius:6px;">
                        <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:8px;">
                            <label style="display:flex;align-items:center;gap:8px;cursor:pointer;">
                                <input type="checkbox" id="notifyEmailEnabled" style="width:16px;height:16px;">
                                <span style="font-size:0.9em;">✉️ <span class="de">E-Mail</span><span class="en">Email</span></span>
                            </label>
                            <button onclick="sendNotifyTestEmail()"
                                style="background:#6c757d;color:white;border:none;padding:6px 14px;border-radius:5px;cursor:pointer;font-size:0.82em;">
                                <span class="de">Test senden</span><span class="en">Send test</span></button>
                        </div>
                        <div style="font-size:0.78em;color:#888;margin-bottom:8px;">
                            <span class="de">Versand per SMTP über ein eigenes Postfach (z.B. Gmail mit App-Passwort — <strong>nicht</strong> das normale Kontopasswort). Port meist 465 (SSL/TLS).</span>
                            <span class="en">Sent via SMTP through your own mailbox (e.g. Gmail with an app password — <strong>not</strong> your regular account password). Port is usually 465 (SSL/TLS).</span>
                        </div>
                        <div style="display:grid;grid-template-columns:1fr 1fr;gap:12px;max-width:600px;margin-bottom:10px;">
                            <div>
                                <div class="status-label">SMTP-Host</div>
                                <input type="text" id="notifySmtpHost" placeholder="smtp.gmail.com" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                            </div>
                            <div>
                                <div class="status-label">Port</div>
                                <input type="number" id="notifySmtpPort" placeholder="465" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                            </div>
                            <div>
                                <div class="status-label"><span class="de">Absender (Login)</span><span class="en">Sender (login)</span></div>
                                <input type="text" id="notifySmtpUser" placeholder="you@gmail.com" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                            </div>
                            <div>
                                <div class="status-label"><span class="de">App-Passwort</span><span class="en">App password</span></div>
                                <input type="text" id="notifySmtpPass" placeholder="unverändert lassen" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                            </div>
                        </div>
                        <div style="max-width:300px;">
                            <div class="status-label"><span class="de">Empfänger</span><span class="en">Recipient</span></div>
                            <input type="text" id="notifySmtpTo" placeholder="you@example.com" style="width:100%;padding:7px;border:1px solid #ddd;border-radius:4px;">
                        </div>
                    </div>
                </div>

                <div style="font-size:0.85em;color:#555;font-weight:bold;margin-bottom:8px;"><span class="de">Einzelne Alarme</span><span class="en">Individual alerts</span></div>
                <div style="display:flex;flex-direction:column;gap:10px;">
                    <div style="display:flex;justify-content:space-between;align-items:center;padding:8px;background:#f8f9fa;border-radius:6px;">
                        <label style="display:flex;align-items:center;gap:8px;cursor:pointer;">
                            <input type="checkbox" id="notifyNetzEnabled" style="width:16px;height:16px;">
                            <span style="font-size:0.9em;">🔌 <span class="de">Netzausfall</span><span class="en">Mains failure</span></span>
                        </label>
                    </div>
                    <div style="display:flex;justify-content:space-between;align-items:center;padding:8px;background:#f8f9fa;border-radius:6px;">
                        <label style="display:flex;align-items:center;gap:8px;cursor:pointer;">
                            <input type="checkbox" id="notifySensorEnabled" style="width:16px;height:16px;">
                            <span style="font-size:0.9em;">⚠️ <span class="de">Sensor-Ausfall</span><span class="en">Sensor failure</span></span>
                        </label>
                    </div>
                    <div style="padding:8px;background:#f8f9fa;border-radius:6px;">
                        <div style="display:flex;justify-content:space-between;align-items:center;">
                            <label style="display:flex;align-items:center;gap:8px;cursor:pointer;">
                                <input type="checkbox" id="notifyFeuchteEnabled" style="width:16px;height:16px;">
                                <span style="font-size:0.9em;">💧 <span class="de">Luftfeuchte außerhalb Bereich</span><span class="en">Humidity out of range</span></span>
                            </label>
                            <span style="font-size:0.78em;color:#888;" id="notifyMs2Hint"></span>
                        </div>
                        <div style="display:flex;gap:8px;align-items:center;margin-top:6px;font-size:0.85em;">
                            <span class="de">Min</span><span class="en">Min</span> <input type="number" id="notifyFeuchteMin" min="0" max="100" style="width:55px;padding:4px;">%
                            <span class="de">Max</span><span class="en">Max</span> <input type="number" id="notifyFeuchteMax" min="0" max="100" style="width:55px;padding:4px;">%
                        </div>
                    </div>
                    <div style="padding:8px;background:#f8f9fa;border-radius:6px;">
                        <div style="display:flex;justify-content:space-between;align-items:center;">
                            <label style="display:flex;align-items:center;gap:8px;cursor:pointer;">
                                <input type="checkbox" id="notifyTemperaturEnabled" style="width:16px;height:16px;">
                                <span style="font-size:0.9em;">🌡️ <span class="de">Temperatur außerhalb Bereich</span><span class="en">Temperature out of range</span></span>
                            </label>
                        </div>
                        <div style="display:flex;gap:8px;align-items:center;margin-top:6px;font-size:0.85em;">
                            <span class="de">Min</span><span class="en">Min</span> <input type="number" id="notifyTemperaturMin" min="0" max="80" style="width:55px;padding:4px;">°C
                            <span class="de">Max</span><span class="en">Max</span> <input type="number" id="notifyTemperaturMax" min="0" max="80" style="width:55px;padding:4px;">°C
                        </div>
                    </div>
                    <div style="padding:8px;background:#f8f9fa;border-radius:6px;">
                        <div style="display:flex;justify-content:space-between;align-items:center;">
                            <label style="display:flex;align-items:center;gap:8px;cursor:pointer;">
                                <input type="checkbox" id="notifyTankEnabled" style="width:16px;height:16px;">
                                <span style="font-size:0.9em;">🪣 <span class="de">Vorratsbehälter fast leer</span><span class="en">Reservoir almost empty</span></span>
                            </label>
                        </div>
                        <div style="display:flex;gap:8px;align-items:center;margin-top:6px;font-size:0.85em;">
                            <span class="de">Grenze</span><span class="en">Threshold</span> <input type="number" id="notifyTankMin" min="0" max="100" style="width:55px;padding:4px;">%
                        </div>
                    </div>
                    <div style="display:flex;justify-content:space-between;align-items:center;padding:8px;background:#f8f9fa;border-radius:6px;">
                        <label style="display:flex;align-items:center;gap:8px;cursor:pointer;">
                            <input type="checkbox" id="notifyLueftmodulEnabled" style="width:16px;height:16px;">
                            <span style="font-size:0.9em;">🌀 <span class="de">Zeltlüfter-Drehzahl auffällig</span><span class="en">Tent fan speed abnormal</span></span>
                        </label>
                    </div>
                </div>

                <div style="display:flex;gap:8px;align-items:center;margin-top:16px;">
                    <button onclick="saveNotifyForm()"
                        style="background:#27ae60;color:white;border:none;padding:9px 22px;border-radius:5px;cursor:pointer;font-size:0.95em;">
                        <span class="de">Speichern</span><span class="en">Save</span></button>
                    <span id="notifySaveStatus" style="font-size:0.85em;color:#888;"></span>
                </div>
            </div>
        </div>
)rawliteral"
