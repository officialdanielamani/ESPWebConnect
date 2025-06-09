document.addEventListener("DOMContentLoaded", () => {
    // Get references to the dashboard and dropdown
    updateReadings();
    closeNotification();
    const dashboard = document.getElementById("dashboard");
    const widgetSelection = document.querySelector(".dropdown-content");

    if (!dashboard || !widgetSelection) {
        console.error("Dashboard or widget selection not found.");
        return;
    }

    // Track active widgets
    const activeWidgets = new Set();

    // Mobile detection
    const isMobile = /Android|webOS|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini/i.test(navigator.userAgent);
    const isTouch = 'ontouchstart' in window || navigator.maxTouchPoints > 0;

    // Touch handling variables
    let touchStartTime = 0;
    let touchMoved = false;
    const TOUCH_THRESHOLD = 10; // pixels
    const LONG_PRESS_DURATION = 500; // milliseconds

    // Lock functionality - MOVED OUTSIDE to be accessible globally
    let isDashboardLocked = false;
    
    // Initialize lock checkbox after DOM is ready
    const lockCheckbox = document.getElementById('lockDashboard');
    if (lockCheckbox) {
        lockCheckbox.addEventListener('change', (e) => {
            isDashboardLocked = e.target.checked;
            updateLockState();
        });
    }

    // Lock state update function
    function updateLockState() {
        const widgets = document.querySelectorAll('.widget');
        widgets.forEach(widget => {
            if (isDashboardLocked) {
                widget.classList.add('locked');
                // Hide controls when locked
                const removeBtn = widget.querySelector('.remove-btn');
                const resizer = widget.querySelector('.resizer');
                const dragHandle = widget.querySelector('.drag-handle');
                
                if (removeBtn) removeBtn.style.display = 'none';
                if (resizer) resizer.style.display = 'none';
                if (dragHandle) dragHandle.style.display = 'none';
            } else {
                widget.classList.remove('locked');
                // Show controls when unlocked
                const removeBtn = widget.querySelector('.remove-btn');
                const resizer = widget.querySelector('.resizer');
                const dragHandle = widget.querySelector('.drag-handle');
                
                if (removeBtn) removeBtn.style.display = '';
                if (resizer) resizer.style.display = '';
                if (dragHandle) dragHandle.style.display = '';
            }
        });
    }

    // Add a new widget
    function addWidget(id, name, type, icon = "fa fa-question", color = "#d4aa4f", desc, unit) {
        if (!dashboard) return;

        // Prevent duplicates
        if (document.querySelector(`#${id}`)) {
            alert(`${name} [${id}] is already on the dashboard.`);
            return;
        }

        const widget = document.createElement("div");
        widget.className = "widget";
        widget.id = id;
        widget.dataset.type = type;
        widget.desc = desc;
        widget.unit = unit;

        // Mobile-friendly initial sizing
        if (isMobile) {
            widget.style.gridColumnStart = 1;
            widget.style.gridRowStart = 1;
            widget.style.gridColumnEnd = "span 6"; // Larger on mobile
            widget.style.gridRowEnd = "span 6";
        } else {
            widget.style.gridColumnStart = 1;
            widget.style.gridRowStart = 1;
            widget.style.gridColumnEnd = "span 4";
            widget.style.gridRowEnd = "span 4";
        }

        const gID = id.toLowerCase();

        // Content generation with improved mobile styling
        if (type === "switch") {
            widget.innerHTML = `
            <div class="card-info">
                <div class="widget-icon"><i class="${icon}" style="color: ${color};"></i></div>
                <div class="widget-name">${name}</div>
                <div class="widget-desc">${desc}</div>
            </div>
            <label class="switch ${isMobile ? 'switch-mobile' : ''}">
                <input type="checkbox" id="${gID}-val" onchange="toggleSwitch('${gID}-val','${id}')">
                <span class="switch_sw"></span>
            </label>
        `;
        } else if (type === "sensor") {
            widget.innerHTML = `
            <div class="card-info">
                <div class="widget-icon"><i class="${icon}" style="color: ${color};"></i></div>
                <div class="widget-name">${name}</div>
                <div class="widget-desc">${desc}</div>
            </div>
            <div class="sensor-value">
                <span class="widget-value" id="${gID}-val">0.00</span>
                <div class="widget-unit">${unit}</div>
            </div>
        `;
        }
        else if (type === "button") {
            widget.innerHTML = `
            <div class="card-info">
                <div class="widget-icon"><i class="${icon}" style="color: ${color};"></i></div>
                <div class="widget-desc">${desc}</div>
            </div>
            <button id="${id}" type="submit" class="${isMobile ? 'button-mobile' : ''}" ontouchstart="handleButtonTouch(event)" onclick="pressButton('${id}')">${name}</button>
        `;
        } else if (type === "inputnumber") {
            widget.innerHTML = `
            <div class="card-info">
                <div class="widget-icon"><i class="${icon}" style="color: ${color};"></i></div>
                <div class="widget-name">${name}</div>
                <div class="widget-desc">${desc}</div>
            </div>
            <div class="widget-form">
            <form onsubmit="return submitInput('${id}')">
                <input type="number" id="${id}" name="value" class="${isMobile ? 'input-mobile' : ''}" inputmode="numeric">
                <button type="submit" class="${isMobile ? 'button-mobile' : ''}">OK</button>
            </form>
            </div>
        `;
        } else if (type === "inputtext") {
            widget.innerHTML = `
            <div class="card-info">
                <div class="widget-icon"><i class="${icon}" style="color: ${color};"></i></div>
                <div class="widget-name">${name}</div>
                <div class="widget-desc">${desc}</div>
            </div>
            <div class="widget-form">
            <form onsubmit="return submitInput('${id}')">
                <input type="text" id="${id}" name="value" class="${isMobile ? 'input-mobile' : ''}">
                <button type="submit" class="${isMobile ? 'button-mobile' : ''}">OK</button>
            </form>
            </div>
        `;
        }

        // Remove button with better mobile support
        const removeBtn = document.createElement("button");
        removeBtn.className = `remove-btn ${isMobile ? 'remove-btn-mobile' : ''}`;
        removeBtn.textContent = "×";
        removeBtn.onclick = (e) => {
            e.stopPropagation();
            removeWidget(id);
        };
        widget.appendChild(removeBtn);

        // Add resizer with touch support
        const resizer = document.createElement("div");
        resizer.className = `resizer ${isMobile ? 'resizer-mobile' : ''}`;
        if (isMobile) {
            resizer.innerHTML = '<i class="fas fa-expand-arrows-alt"></i>';
        }
        widget.appendChild(resizer);

        // Add drag handle for mobile
        if (isMobile) {
            const dragHandle = document.createElement("div");
            dragHandle.className = "drag-handle";
            dragHandle.innerHTML = '<i class="fas fa-grip-vertical"></i>';
            widget.appendChild(dragHandle);
        }

        // Add drag-and-resize functionality
        enableDragAndResize(widget);

        // Add to the dashboard
        dashboard.appendChild(widget);
        activeWidgets.add(id);

        // Apply current lock state to new widget
        if (isDashboardLocked) {
            setTimeout(() => updateLockState(), 0);
        }
    }

    // Remove a widget
    function removeWidget(id) {
        const widget = document.getElementById(id);
        if (widget) {
            widget.remove();
            activeWidgets.delete(id);
        }
    }

    // Enhanced drag and resize with better touch support
    function enableDragAndResize(widget) {
        let isDragging = false;
        let isResizing = false;
        let startX, startY, initialWidth, initialHeight;
        let dragStartX, dragStartY;

        const maxColumns = 32;
        const maxRows = 32;
        const gridWidth = isMobile ? 40 : 50; // Smaller grid on mobile
        const gridHeight = isMobile ? 40 : 50;

        // Improved touch event handling
        const getTouchEvent = (e) => {
            return e.touches ? e.touches[0] : e.changedTouches ? e.changedTouches[0] : e;
        };

        // Prevent scrolling during drag operations on mobile
        const preventScroll = (e) => {
            if (isDragging || isResizing) {
                e.preventDefault();
            }
        };

        // Enhanced dragging logic
        const startDrag = (e) => {
            if (isResizing || isDashboardLocked) return; // Lock check here
            
            const touch = getTouchEvent(e);
            touchStartTime = Date.now();
            touchMoved = false;
            dragStartX = touch.clientX;
            dragStartY = touch.clientY;

            // On mobile, require drag handle or long press
            if (isMobile) {
                const dragHandle = widget.querySelector('.drag-handle');
                const isFromHandle = dragHandle && dragHandle.contains(e.target);
                
                if (!isFromHandle) {
                    // Start long press timer
                    const longPressTimer = setTimeout(() => {
                        if (!touchMoved) {
                            startDragOperation(e);
                        }
                    }, LONG_PRESS_DURATION);

                    const cancelLongPress = () => {
                        clearTimeout(longPressTimer);
                        document.removeEventListener('touchmove', cancelLongPress);
                        document.removeEventListener('touchend', cancelLongPress);
                    };

                    document.addEventListener('touchmove', cancelLongPress);
                    document.addEventListener('touchend', cancelLongPress);
                    return;
                }
            }

            startDragOperation(e);
        };

        const startDragOperation = (e) => {
            isDragging = true;
            widget.classList.add("dragging");
            
            const touch = getTouchEvent(e);
            startX = touch.clientX;
            startY = touch.clientY;

            // Add visual feedback
            if (isMobile) {
                widget.style.opacity = '0.8';
                widget.style.transform = 'scale(1.05)';
            }

            document.addEventListener('mousemove', moveDrag, { passive: false });
            document.addEventListener('mouseup', stopDrag);
            document.addEventListener('touchmove', moveDrag, { passive: false });
            document.addEventListener('touchend', stopDrag);
            
            if (isMobile) {
                document.addEventListener('touchmove', preventScroll, { passive: false });
            }
        };

        const moveDrag = (e) => {
            if (!isDragging) return;
            
            const touch = getTouchEvent(e);
            const deltaX = Math.abs(touch.clientX - dragStartX);
            const deltaY = Math.abs(touch.clientY - dragStartY);
            
            if (deltaX > TOUCH_THRESHOLD || deltaY > TOUCH_THRESHOLD) {
                touchMoved = true;
            }

            const rect = dashboard.getBoundingClientRect();

            // Calculate new grid positions
            let column = Math.max(1, Math.ceil((touch.clientX - rect.left) / gridWidth));
            let row = Math.max(1, Math.ceil((touch.clientY - rect.top) / gridHeight));

            // Get current span values
            const columnSpan = parseInt(widget.style.gridColumnEnd.split("span")[1]) || 4;
            const rowSpan = parseInt(widget.style.gridRowEnd.split("span")[1]) || 4;

            // Constrain to grid boundaries
            column = Math.min(column, maxColumns - columnSpan + 1);
            row = Math.min(row, maxRows - rowSpan + 1);

            widget.style.gridColumnStart = column;
            widget.style.gridRowStart = row;

            e.preventDefault();
        };

        const stopDrag = (e) => {
            if (!isDragging) return;
            
            isDragging = false;
            widget.classList.remove("dragging");
            
            // Remove visual feedback
            if (isMobile) {
                widget.style.opacity = '';
                widget.style.transform = '';
            }

            document.removeEventListener('mousemove', moveDrag);
            document.removeEventListener('mouseup', stopDrag);
            document.removeEventListener('touchmove', moveDrag);
            document.removeEventListener('touchend', stopDrag);
            document.removeEventListener('touchmove', preventScroll);
        };

        // Enhanced resizing logic
        const startResize = (e) => {
            if (isDashboardLocked) return; // Lock check here
            e.preventDefault();
            e.stopPropagation();
            
            isResizing = true;
            const touch = getTouchEvent(e);
            startX = touch.clientX;
            startY = touch.clientY;

            const rect = widget.getBoundingClientRect();
            initialWidth = rect.width;
            initialHeight = rect.height;

            // Visual feedback for resize
            if (isMobile) {
                widget.style.outline = '2px dashed var(--button-background-color)';
            }

            document.addEventListener('mousemove', moveResize, { passive: false });
            document.addEventListener('mouseup', stopResize);
            document.addEventListener('touchmove', moveResize, { passive: false });
            document.addEventListener('touchend', stopResize);
            
            if (isMobile) {
                document.addEventListener('touchmove', preventScroll, { passive: false });
            }
        };

        const moveResize = (e) => {
            if (!isResizing) return;
            
            const touch = getTouchEvent(e);
            const dx = touch.clientX - startX;
            const dy = touch.clientY - startY;

            // Calculate new spans with minimum sizes
            const minSpan = isMobile ? 3 : 2;
            let newWidth = Math.max(minSpan, Math.ceil((initialWidth + dx) / gridWidth));
            let newHeight = Math.max(minSpan, Math.ceil((initialHeight + dy) / gridHeight));

            // Constrain spans to grid boundaries
            const currentColumn = parseInt(widget.style.gridColumnStart) || 1;
            const currentRow = parseInt(widget.style.gridRowStart) || 1;
            
            newWidth = Math.min(newWidth, maxColumns - currentColumn + 1);
            newHeight = Math.min(newHeight, maxRows - currentRow + 1);

            widget.style.gridColumnEnd = `span ${newWidth}`;
            widget.style.gridRowEnd = `span ${newHeight}`;

            e.preventDefault();
        };

        const stopResize = (e) => {
            if (!isResizing) return;
            
            isResizing = false;
            
            // Remove visual feedback
            if (isMobile) {
                widget.style.outline = '';
            }

            document.removeEventListener('mousemove', moveResize);
            document.removeEventListener('mouseup', stopResize);
            document.removeEventListener('touchmove', moveResize);
            document.removeEventListener('touchend', stopResize);
            document.removeEventListener('touchmove', preventScroll);
        };

        // Event listeners with touch support
        const dragTarget = isMobile ? (widget.querySelector('.drag-handle') || widget) : widget;
        
        dragTarget.addEventListener("mousedown", startDrag);
        dragTarget.addEventListener("touchstart", startDrag, { passive: false });

        const resizer = widget.querySelector(".resizer");
        if (resizer) {
            resizer.addEventListener("mousedown", startResize);
            resizer.addEventListener("touchstart", startResize, { passive: false });
        }

        // Prevent context menu on long press for widgets
        if (isMobile) {
            widget.addEventListener('contextmenu', (e) => {
                e.preventDefault();
            });
        }
    }

    // Event delegation for dropdown with touch improvements
    widgetSelection.addEventListener("click", (e) => {
        const button = e.target.closest("button");
        if (!button || !button.dataset.widget) return;

        const id = button.dataset.widget;
        const name = button.dataset.name || "Unknown Widget";
        const type = button.dataset.type || "sensor";
        const icon = button.dataset.icon || "fa fa-question";
        const color = button.dataset.color || "#d4aa4f";
        const desc = button.dataset.desc || "";
        const unit = button.dataset.unit || "";

        addWidget(id, name, type, icon, color, desc, unit);
        
        // Close dropdown on mobile after selection
        if (isMobile) {
            const dropdown = button.closest('.dropdown');
            if (dropdown) {
                dropdown.querySelector('.dropdown-content').style.display = 'none';
            }
        }
    });

    // Improved dropdown behavior for mobile
    if (isMobile) {
        document.querySelectorAll('.dropdown').forEach(dropdown => {
            const button = dropdown.querySelector('button');
            const content = dropdown.querySelector('.dropdown-content');
            
            button.addEventListener('click', (e) => {
                e.stopPropagation();
                const isVisible = content.style.display === 'block';
                
                // Close all other dropdowns
                document.querySelectorAll('.dropdown-content').forEach(dc => {
                    dc.style.display = 'none';
                });
                
                // Toggle current dropdown
                content.style.display = isVisible ? 'none' : 'block';
            });
        });

        // Close dropdowns when clicking outside
        document.addEventListener('click', () => {
            document.querySelectorAll('.dropdown-content').forEach(dc => {
                dc.style.display = 'none';
            });
        });
    }

    function loadDashboard() {
        const savedState = JSON.parse(localStorage.getItem("dashboardSettings"));
        if (!savedState) return;
    
        const validWidgets = Array.from(widgetSelection.querySelectorAll("button")).map(
            (btn) => btn.dataset.widget
        );
    
        savedState.widgets.forEach((widgetData) => {
            if (!validWidgets.includes(widgetData.id)) return;
    
            addWidget(
                widgetData.id,
                widgetData.name || widgetData.id,
                widgetData.type,
                widgetData.icon,
                widgetData.color,
                widgetData.desc,
                widgetData.unit
            );
    
            const widget = document.getElementById(widgetData.id);
            if (widget) {
                widget.style.gridColumnStart = widgetData.position.columnStart;
                widget.style.gridColumnEnd = widgetData.position.columnEnd;
                widget.style.gridRowStart = widgetData.position.rowStart;
                widget.style.gridRowEnd = widgetData.position.rowEnd;
            }
        });
    
        if (savedState.theme) {
            document.body.dataset.theme = savedState.theme;
        }

        // Restore lock state
        if (savedState.locked !== undefined) {
            isDashboardLocked = savedState.locked;
            if (lockCheckbox) {
                lockCheckbox.checked = isDashboardLocked;
            }
            setTimeout(() => updateLockState(), 100);
        }
    }
    
    // Make functions global by attaching to window
    window.saveDashboard = function() {
        const widgets = Array.from(dashboard.querySelectorAll(".widget"));
        const savedWidgets = widgets.map((widget) => {
            let name = widget.querySelector('.btn-widget')?.innerText ||
                       widget.querySelector('.widget-name')?.innerText ||
                       widget.querySelector('button[type="submit"]')?.innerText ||
                       widget.id;

            return {
                id: widget.id,
                name: name,
                type: widget.dataset.type || "undefined",
                icon: widget.querySelector('.widget-icon i')?.className || "fa fa-question",
                color: widget.querySelector('.widget-icon i')?.style.color || "#d4aa4f",
                desc: widget.querySelector('.widget-desc')?.innerText || "",
                unit: widget.querySelector('.widget-unit')?.innerText || "",
                position: {
                    columnStart: widget.style.gridColumnStart,
                    columnEnd: widget.style.gridColumnEnd,
                    rowStart: widget.style.gridRowStart,
                    rowEnd: widget.style.gridRowEnd,
                },
            };
        });

        const theme = document.body.dataset.theme || "default";
        const dashboardState = { 
            widgets: savedWidgets, 
            theme,
            locked: isDashboardLocked // Save lock state
        };

        localStorage.setItem("dashboardSettings", JSON.stringify(dashboardState));
        alert("Dashboard saved!");
    };

    window.clearDashboard = function() {
        localStorage.removeItem("dashboardSettings");
        dashboard.innerHTML = "";
        alert("Dashboard cleared!");
    };

    loadDashboard();
});

// Enhanced mobile button handling
function handleButtonTouch(event) {
    event.target.style.transform = 'scale(0.95)';
    setTimeout(() => {
        event.target.style.transform = '';
    }, 150);
}

// Enhanced toggle switch with better touch feedback
function toggleSwitch(gID, id) {
    var checkbox = document.querySelector(`input#${gID}`);
    if (!checkbox) {
        console.error(`Checkbox with ID '${gID}' not found`);
        return;
    }

    // Add haptic feedback on supported devices
    if (navigator.vibrate) {
        navigator.vibrate(50);
    }

    var xhr = new XMLHttpRequest();
    var state = checkbox.checked ? 'true' : 'false';
    xhr.open('GET', `/toggleSwitch?id=${id}&state=${state}`, true);
    xhr.send();
}

// Enhanced button press with touch feedback
function pressButton(id) {
    // Add haptic feedback
    if (navigator.vibrate) {
        navigator.vibrate(100);
    }

    var xhr = new XMLHttpRequest();
    xhr.open('GET', '/pressButton?id=' + id, true);
    xhr.send();
}

// Enhanced form submission with better mobile UX
function submitInput(id) {
    const inputElement = document.querySelector(`form[onsubmit="return submitInput('${id}')"] input`);
    
    if (!inputElement) {
        console.error(`Input field for id '${id}' not found.`);
        return false;
    }

    const value = inputElement.value;

    // Show loading state
    const submitButton = inputElement.parentNode.querySelector('button[type="submit"]');
    const originalText = submitButton.textContent;
    submitButton.textContent = '...';
    submitButton.disabled = true;

    fetch('/' + id, {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: 'value=' + encodeURIComponent(value)
    })
        .then(response => response.text())
        .then(text => {
            console.log(text);
            
            // Visual feedback
            submitButton.textContent = '✓';
            setTimeout(() => {
                submitButton.textContent = originalText;
                submitButton.disabled = false;
            }, 1000);

            // Clear input on success
            inputElement.value = '';
            
            // Blur input to hide mobile keyboard
            inputElement.blur();
        })
        .catch(error => {
            console.error('Error:', error);
            submitButton.textContent = '✗';
            setTimeout(() => {
                submitButton.textContent = originalText;
                submitButton.disabled = false;
            }, 1000);
        });

    return false;
}

// WebSocket with better error handling
const webSocket = new WebSocket(`ws://${window.location.hostname}/ws`);

webSocket.onmessage = (event) => {
    try {
        const { id, message, messageColor, icon, iconColor, timeout } = JSON.parse(event.data);
        showNotification(message, messageColor, icon, iconColor, timeout);
    } catch (error) {
        console.error('Error parsing WebSocket message:', error);
    }
};

webSocket.onerror = (error) => {
    console.error('WebSocket error:', error);
};

// Enhanced notification system
function showNotification(message, messageColor = '', icon = '', iconColor = '', timeout = 0) {
    const banner = document.getElementById('notificationBanner');
    const messageElement = document.getElementById('notificationMessage');
    const iconElement = document.getElementById('notificationIcon');

    messageElement.textContent = message;
    if (messageColor) {
        messageElement.style.color = messageColor;
        iconElement.style.color = messageColor;
    }

    if (icon) {
        iconElement.className = `notification-icon ${icon}`;
    }

    if (iconColor) {
        banner.style.backgroundColor = iconColor;
    }

    banner.style.display = 'flex';

    // Add haptic feedback for notifications on mobile
    if (navigator.vibrate && /Android|webOS|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini/i.test(navigator.userAgent)) {
        navigator.vibrate([100, 50, 100]);
    }

    if (timeout > 0) {
        setTimeout(() => {
            banner.style.display = 'none';
        }, timeout * 1000);
    }
}

function closeNotification() {
    const banner = document.getElementById('notificationBanner');
    banner.style.removeProperty('display');
    banner.style.display = 'none';
}

// Theme selection with better mobile support
function changeTheme(event) {
    const selectedTheme = event.target.value;
    document.body.setAttribute('data-theme', selectedTheme);
    
    // Close dropdown on mobile
    if (/Android|webOS|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini/i.test(navigator.userAgent)) {
        const dropdown = event.target.closest('.dropdown');
        if (dropdown) {
            dropdown.querySelector('.dropdown-content').style.display = 'none';
        }
    }
}