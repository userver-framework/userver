import { initializeApp } from "https://www.gstatic.com/firebasejs/10.12.2/firebase-app.js";
import {
  getDatabase,
  ref,
  runTransaction,
} from "https://www.gstatic.com/firebasejs/10.12.2/firebase-database.js";

const FEEDBACK_FORM_LINK =
  "https://forms.yandex.ru/u/667d482fe010db2f53e00edf/";

const firebaseConfig = Object.freeze({
  apiKey: "AIzaSyDZVAx6BhwsFIvTe0JbBP9lf8VULw7Co6s",
  authDomain: "userver-test-74c30.firebaseapp.com",
  projectId: "userver-test-74c30",
  storageBucket: "userver-test-74c30.appspot.com",
  messagingSenderId: "949186490366",
  appId: "1:949186490366:web:838a0823848dcb5a69daaf",
  measurementId: "G-PSNRVCVKCL",
});
const firebaseApp = initializeApp(firebaseConfig);

const firebasePageFeedbackActions = Object.freeze({
  like: "like",
  dislike: "dislike",
});

export class PageFeedback extends HTMLElement {
  #likeCheckbox = this.#makeFeedbackCheckbox({
    className: "like-checkbox",
    title: "Helpful",
  });
  #dislikeCheckbox = this.#makeFeedbackCheckbox({
    className: "dislike-checkbox",
    title: "Not helpful",
  });

  #popup = document.createElement("page-feedback-popup");

  #docContentElement = document.getElementById("doc-content");
  #contentsElement = document.querySelector(".contents");
  #resizeObserver = new ResizeObserver(this.#updatePosition.bind(this));

  #title = document.querySelector(".title");

  get #pageTitle() {
    return this.#title.textContent.replace(/[.$#[\]/]/g, "-");
  }

  static init() {
    const toc = document.querySelector(".toc");

    if (toc === null) {
      return;
    } else {
      toc.parentNode.insertBefore(document.createElement("page-feedback"), toc);
    }
  }

  constructor() {
    super();
    this.#addEventListeners();
  }

  #addEventListeners() {
    this.#likeCheckbox.addEventListener(
      "change",
      this.#getCheckboxChangeCallback({
        checkbox: this.#likeCheckbox,
        oppositeCheckbox: this.#dislikeCheckbox,
        modalOffset: -152,
        title: "Thank you!",
        actionName: firebasePageFeedbackActions.like,
      })
    );
    this.#dislikeCheckbox.addEventListener(
      "change",
      this.#getCheckboxChangeCallback({
        checkbox: this.#dislikeCheckbox,
        oppositeCheckbox: this.#likeCheckbox,
        modalOffset: -125,
        title: "Tell us what's wrong",
        actionName: firebasePageFeedbackActions.dislike,
      })
    );
  }

  #getCheckboxChangeCallback({
    checkbox,
    oppositeCheckbox,
    modalOffset,
    title,
    actionName,
  }) {
    return () => {
      if (checkbox.checked) {
        this.#handleCheckedAction({
          oppositeCheckbox,
          modalOffset,
          title,
          actionName,
        });
      } else if (!oppositeCheckbox.checked) {
        this.#handleUncheckedAction(actionName);
      }
    };
  }

  #handleCheckedAction({ oppositeCheckbox, modalOffset, title, actionName }) {
    this.#popup.open({ modalOffset, title });
    if (oppositeCheckbox.checked) {
      this.#switchFirebaseDatabaseFeedbackField(actionName);
    } else {
      FirebasePageDatabase.incrementField(this.#pageTitle, actionName);
    }
    oppositeCheckbox.checked = false;
  }

  #switchFirebaseDatabaseFeedbackField(field) {
    const { like: likeField, dislike: dislikeField } =
      firebasePageFeedbackActions;
    const oppositeField = field === likeField ? dislikeField : likeField;
    FirebasePageDatabase.incrementField(this.#pageTitle, field);
    FirebasePageDatabase.decrementField(this.#pageTitle, oppositeField);
  }

  #handleUncheckedAction(actionName) {
    FirebasePageDatabase.decrementField(this.#pageTitle, actionName);
    this.#popup.close();
  }

  connectedCallback() {
    this.appendChild(this.#likeCheckbox);
    this.appendChild(this.#dislikeCheckbox);
    this.appendChild(this.#popup);

    this.#resizeObserver.observe(document.body);
  }

  disconnectedCallback() {
    this.#resizeObserver.unobserve(document.body);
  }

  #makeFeedbackCheckbox({ className, title }) {
    const checkbox = document.createElement("input");
    checkbox.type = "checkbox";
    checkbox.title = title;
    checkbox.className = className;
    return checkbox;
  }

  #updatePosition() {
    const marginRight = parseFloat(
      window.getComputedStyle(this.#contentsElement).marginRight
    );
    const paddingRight = parseFloat(
      window.getComputedStyle(this.#contentsElement).paddingRight
    );
    const scrollbarWidth =
      this.#docContentElement.offsetWidth - this.#docContentElement.clientWidth;
    this.style.right = `${marginRight + paddingRight + scrollbarWidth}px`;
  }
}

class PageFeedbackPopup extends HTMLElement {
  static #inactiveCloseTimeMilliseconds = 5000;

  #title = document.createElement("h6");
  #paragraph = this.#makeParagraph(
    "Your opinion will help to improve our service"
  );
  #link = this.#makeFeedbackLink(FEEDBACK_FORM_LINK);

  #currentFadeOutAnimation = null;

  #inactivityCloseTimerID = null;
  #clickOutsidePopupHandlerCallback = null;

  get #isOpen() {
    return this.style.opacity >= 1;
  }

  constructor() {
    super();
    this.#addEventListeners();
  }

  #addEventListeners() {
    this.addEventListener(
      "mouseenter",
      this.#removeInactivityCloseTimer.bind(this)
    );
    this.addEventListener(
      "mouseleave",
      this.#startInactivityCloseTimer.bind(this)
    );
  }

  connectedCallback() {
    this.appendChild(this.#title);
    this.appendChild(this.#paragraph);
    this.appendChild(this.#link);
  }

  async open({ modalOffset, title }) {
    if (this.#isOpen) {
      await this.close();
    }

    this.#title.textContent = title;
    this.style.left = `${modalOffset}px`;
    await this.#fadeIn();
    this.#startInactivityCloseTimer();
    this.#addClickOutsidePopupCloseHandler();
  }

  #startInactivityCloseTimer() {
    if (this.#inactivityCloseTimerID === null) {
      this.#inactivityCloseTimerID = setInterval(
        this.close.bind(this),
        PageFeedbackPopup.#inactiveCloseTimeMilliseconds
      );
    } else {
      return;
    }
  }

  #addClickOutsidePopupCloseHandler() {
    this.#clickOutsidePopupHandlerCallback = (event) => {
      const elementsUnderCursor = document.elementsFromPoint(
        event.clientX,
        event.clientY
      );
      const isPopupUnderCursor = elementsUnderCursor.some(
        (element) => element === this
      );
      if (!isPopupUnderCursor) {
        this.close();
      }
    };

    document.addEventListener("click", this.#clickOutsidePopupHandlerCallback);
  }

  async #fadeIn() {
    return new Promise((resolve) => {
      let opacity = 0;
      this.style.zIndex = 10;
      const timer = setInterval(() => {
        if (opacity < 1) {
          opacity += 0.05;
          this.style.opacity = opacity;
        } else {
          this.style.opacity = 1;
          clearInterval(timer);
          resolve();
        }
      }, 6);
    });
  }

  async close() {
    this.#removeInactivityCloseTimer();
    this.#removeClickOutsidePopupCloseHandler();
    await this.#fadeOut();
  }

  #removeInactivityCloseTimer() {
    clearInterval(this.#inactivityCloseTimerID);
    this.#inactivityCloseTimerID = null;
  }

  #removeClickOutsidePopupCloseHandler() {
    document.removeEventListener(
      "click",
      this.#clickOutsidePopupHandlerCallback
    );
    this.#clickOutsidePopupHandlerCallback = null;
  }

  async #fadeOut() {
    if (this.#currentFadeOutAnimation === null) {
      return this.#startFadeOutAnimation();
    } else {
      return this.#currentFadeOutAnimation;
    }
  }

  #startFadeOutAnimation() {
    this.#currentFadeOutAnimation = new Promise((resolve) => {
      let opacity = this.style.opacity;
      const timer = setInterval(() => {
        if (opacity > 0) {
          opacity -= 0.05;
          this.style.opacity = opacity;
        } else {
          this.style.zIndex = 0;
          this.style.opacity = 0;
          clearInterval(timer);
          this.#currentFadeOutAnimation = null;
          resolve();
        }
      }, 6);
    });

    return this.#currentFadeOutAnimation;
  }

  #makeParagraph(text) {
    const paragraph = document.createElement("p");
    paragraph.textContent = text;
    return paragraph;
  }

  #makeFeedbackLink(href) {
    const link = document.createElement("a");
    link.href = href;
    link.target = "_blank";
    link.textContent = "Leave a feedback >";
    link.title = "Fill out the feedback form";
    return link;
  }
}

class LandingFeedback {
  static #lastStarRatingLabel = null;

  static init() {
    this.#updateFormLinkInFeedbackButton();
    this.#addStarFeedbackMetricSendOnChangeEventListeners();
  }

  static #updateFormLinkInFeedbackButton() {
    const feedbackButton = document.querySelector(".feedback__button");

    if (feedbackButton !== null) {
      feedbackButton.href = FEEDBACK_FORM_LINK;
    }
  }

  static #addStarFeedbackMetricSendOnChangeEventListeners() {
    const starFeedbackRadios = document.querySelectorAll(".feedback__star");

    starFeedbackRadios.forEach((star) => {
      star.addEventListener("change", () => {
        if (this.#lastStarRatingLabel !== null) {
          FirebasePageDatabase.decrementField(
            "Landing",
            this.#lastStarRatingLabel
          );
        }

        FirebasePageDatabase.incrementField("Landing", star.ariaLabel);
        this.#lastStarRatingLabel = star.ariaLabel;
      });
    });
  }
}

class FirebasePageDatabase {
  static incrementField(pageTitle, field) {
    this.#doAction({
      callback: (page) => this.#incrementPageField(page, field),
      pageTitle: pageTitle,
    });
  }

  static #incrementPageField(page, field) {
    if (page[field] === undefined) {
      page[field] = 1;
    } else {
      page[field]++;
    }
  }

  static decrementField(pageTitle, field) {
    this.#doAction({
      callback: (page) => this.#decrementPageField(page, field),
      pageTitle: pageTitle,
    });
  }

  static #decrementPageField(page, field) {
    if (page[field] === undefined) {
      page[field] = 0;
    } else {
      page[field]--;
    }
  }

  static #doAction({ callback, pageTitle }) {
    const db = getDatabase(firebaseApp);
    const pageRef = ref(db, `feedback/${pageTitle}`);

    runTransaction(pageRef, (page) => {
      if (page === null) {
        page = {};
      }
      callback(page);
      return page;
    });
  }
}

customElements.define("page-feedback", PageFeedback);
customElements.define("page-feedback-popup", PageFeedbackPopup);

$(function () {
  $(document).ready(function () {
    setTimeout(() => {
      const isLanding = document.getElementById("landing_logo_id") !== null;

      if (isLanding) {
        LandingFeedback.init();
      } else {
        PageFeedback.init();
      }
    }, 0);
  });
});
