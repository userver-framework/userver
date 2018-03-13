#pragma once

#include <string>

namespace storages {
namespace mongo {

inline std::string CompoundKey(std::string key) { return std::move(key); }

template <typename... Strings>
std::string CompoundKey(std::string head, Strings&&... tail) {
  return std::move(head) + '.' + CompoundKey(std::forward<Strings>(tail)...);
}

namespace db {

namespace Corp {

static const std::string kDb = "dbcorp";
namespace Clients {
static const std::string kId = "_id";
static const std::string kIsActive = "is_active";
static const std::string kCabinetOnlyRoleId = "cabinet_only_role_id";
static const std::string kTz = "tz";
static const std::string kComment = "comment";
}  // namespace Clients

namespace Users {
static const std::string kId = "_id";
static const std::string kClientId = "client_id";
static const std::string kPhoneId = "phone_id";
static const std::string kIsActive = "is_active";
static const std::string kCostCenter = "cost_center";
static const std::string kStat = "stat";
static const std::string kStatTimestringFormat = "%Y-%m";
static const std::string kSpentWithVAT = "spent_with_vat";
static const std::string kRole = "role";
namespace Role {
static const std::string kRoleId = "role_id";
static const std::string kLimit = "limit";
static const std::string kClasses = "classes";
}  // namespace Role
}  // namespace Users

namespace Roles {
static const std::string kId = "_id";
static const std::string kClientId = "client_id";
static const std::string kName = "name";
static const std::string kLimit = "limit";
static const std::string kClasses = "classes";
}  // namespace Roles
}  // namespace Corp

namespace Taxi {
static const std::string kDb = "dbtaxi";
static const std::string kDbProcessing = "dbprocessing";

namespace Static {
static const std::string kCollection = kDb + ".static";

static const std::string kId = "_id";

static const std::string kOrderRestrictionId = "order_restriction";
}  // namespace Static

namespace Orderlocks {  // All the same for phone/card/device locks
static const std::string kId = "_id";
static const std::string kItemId = "i";
static const std::string kUnfinishedOrderIds = "o";
static const std::string kUnpaidOrderIds = "x";
}  // namespace Orderlocks

namespace Config {
static const std::string kCollection = kDb + ".config";

static const std::string kId = "_id";
static const std::string kUpdated = "updated";
}  // namespace Config

namespace Orders {
static const std::string kCollection = kDb + ".orders";

static const std::string kId = "_id";
static const std::string kType = "_type";
static const std::string kClientNotifications = "client_notifications";
static const std::string kStatus = "status";
static const std::string kStatusUpdated = "status_updated";
static const std::string kCity = "city";
static const std::string kNearestZone = "nz";
static const std::string kCreated = "created";
static const std::string kUpdated = "updated";
static const std::string kDue = "due";
static const std::string kReorder = "reorder";
static const std::string kUserId = "user_id";
static const std::string kDeviceId = "device_id";
static const std::string kUserPhoneId = "user_phone_id";
static const std::string kUserUid = "user_uid";
static const std::string kCommitState = "commit_state";
static const std::string kTerminalId = "terminal_id";
static const std::string kOperatorLogin = "operator_login";
static const std::string kVendorId = "vendor_id";
static const std::string kUserLocale = "user_locale";
static const std::string kUserAgent = "user_agent";
static const std::string kRequest = "request";
static const std::string kCreditcard = "creditcard";
static const std::string kStatistics = "statistics";
static const std::string kBillingContract = "billing_contract";
static const std::string kCoupon = "coupon";
static const std::string kPaymentTech = "payment_tech";
static const std::string kBillingTech = "billing_tech";
static const std::string kStatusChangePositions = "status_change_positions";
static const std::string kDontCall = "dont_call";
static const std::string kReferralId = "referral_id";
static const std::string kReferralTimestamp = "referral_timestamp";
static const std::string kDontSms = "dont_sms";
static const std::string kBilling = "billing";
static const std::string kFeedback = "feedback";
static const std::string kSource = "source";
static const std::string kCheckOffer = "check_offer";
static const std::string kSvoCarNumber = "svo_car_number";
static const std::string kTaxiStatus = "taxi_status";
static const std::string kExperiments = "experiments";
static const std::string kCost = "cost";
static const std::string kCalc = "calc";
static const std::string kDiscount = "discount";
static const std::string kFixedPrice = "fixed_price";
static const std::string kPerformer = "performer";
static const std::string kRoute = "route";
static const std::string kChatId = "chat_id";
static const std::string kNeedSyncGrades = "need_sync_grades";

static const std::string kVersion = "version";
static const std::string kPaymentEvents = "payment_events";

namespace CalcInfo {
static const std::string kAllowedTariffs = "allowed_tariffs";
static const std::string kDistance = "distance";
static const std::string kTime = "time";
static const std::string kRecalculated = "recalculated";
static const std::string kExtraDistanceMultiplier = "extra_distance_multiplier";
static const std::string kPark = "__park__";
}  // namespace CalcInfo

namespace FixedPrice {
static const std::string kPrice = "price";
static const std::string kDestination = "destination";
static const std::string kMaxDistanceFromB = "max_distance_from_b";
static const std::string kShowPriceInTaximeter = "show_price_in_taximeter";
}  // namespace FixedPrice

namespace Reorder {
static const std::string kId = "id";
}  // namespace Reorder

namespace Requests {
static const std::string kDue = "due";
static const std::string kSource = "source";
static const std::string kSourceRaw = "source_raw";
static const std::string kSourceGeoareas = "source_geoareas";
static const std::string kDestinations = "destinations";
static const std::string kRequirements = "requirements";
static const std::string kCommitState = "commit_state";
static const std::string kClasses = "class";
static const std::string kSurgeUpgradeClasses = "surge_upgrade_classes";
static const std::string kExcludedParks = "excluded_parks";
static const std::string kIgnoreExcludedParks = "ignore_excluded_parks";
static const std::string kComment = "comment";
static const std::string kCorp = "corp";
static const std::string kUpdatedRequirements = "updated_requirements";
static const std::string kSelectedPromotions = "selected_promotions";
static const std::string kPayment = "payment";
static const std::string kServiceLevel = "service_level";
static const std::string kSurgePrice = "sp";
static const std::string kSurgePriceRequired = "spr";
static const std::string kDriverClientChatEnabled = "driverclientchat_enabled";
static const std::string kOfferId = "offer";
static const std::string kUserAgent = "user_agent";
static const std::string kPassengersCount = "passengers_count";
static const std::string kLuggageCount = "luggage_count";

namespace Source {
static const std::string kPorchNumber = "porchnumber";
}

namespace Corp {
static const std::string kCostCenter = "cost_center";
static const std::string kUserId = "user_id";
static const std::string kClientComment = "client_comment";
static const std::string kClientId = "client_id";
}  // namespace Corp

namespace UpdatedRequirement {
static const std::string kRequirement = "requirement";
static const std::string kValue = "value";
static const std::string kReason = "reason";

namespace Reason {
static const std::string kCode = "code";
}  // namespace Reason
}  // namespace UpdatedRequirement

namespace Payment {
static const std::string kType = "type";
static const std::string kPaymentMethodId = "payment_method_id";
}  // namespace Payment

namespace Requirement {
static const std::string kCreditcard = "creditcard";
static const std::string kCorp = "corp";
}  // namespace Requirement

}  // namespace Requests

namespace PaymentEvent {
static const std::string kCreated = "c";
static const std::string kType = "type";
static const std::string kStatus = "status";
static const std::string kData = "data";
static const std::string kFrom = "from";
static const std::string kTo = "to";
static const std::string kReason = "reason";
static const std::string kCode = "code";
static const std::string kPaymentMethodId = "payment_method_id";
static const std::string kText = "text";

namespace Data {
static const std::string kFrom = "from";
static const std::string kTo = "to";
static const std::string kReason = "reason";
static const std::string kType = "type";

namespace Reason {
static const std::string kCode = "code";
}  // namespace Reason
}  // namespace Data
}  // namespace PaymentEvent

namespace Creditcard {
static const std::string kTipsPercent = "tips_perc_default";
static const std::string kCredentials = "credentials";

namespace Credentials {
static const std::string kCardNumber = "card_number";
static const std::string kCardSystem = "card_system";
}  // namespace Credentials
}  // namespace Creditcard

namespace Performer {
static const std::string kClid = "clid";
static const std::string kUuid = "uuid";
static const std::string kDriverLicense = "driver_license";
static const std::string kTariff = "tariff";

namespace Tariff {
static const std::string kId = "id";
static const std::string kCategoryId = "ci";
static const std::string kClass = "class";
static const std::string kCurrency = "currency";
}  // namespace Tariff
}  // namespace Performer

namespace Statistics {
static const std::string kApplication = "application";
static const std::string kUrgency = "urgency";
static const std::string kCancelTime = "cancel_time";
static const std::string kCancelDistance = "cancel_distance";
static const std::string kDriverDelay = "driver_delay";
static const std::string kDriverArrived = "driver_arrived";
static const std::string kFake = "fake";
static const std::string kLateCancel = "late_cancel";
static const std::string kCarackTime = "carack_time";
static const std::string kRouteDistance = "route_distance";
static const std::string kRouteTime = "route_time";
static const std::string kStartWaitingTime = "start_waiting_time";
static const std::string kStartTransportingTime = "start_transporting_time";
static const std::string kTravelTime = "travel_time";
static const std::string kCompleteTime = "complete_time";
static const std::string kCalibrate = "calibrate";
static const std::string kFakeWaiting = "fake_waiting";
static const std::string kSureToolate = "sure_toolate";
static const std::string kOnline = "online";

namespace Online {
static const std::string kFresh = "fresh";
static const std::string kOverdue = "overdue";
static const std::string kMissing = "missing";
}  // namespace Online
}  // namespace Statistics

namespace BillingContract {
static const std::string kIsSet = "is_set";
}  // namespace BillingContract

namespace Coupon {
static const std::string kId = "id";
static const std::string kUsed = "was_used";
static const std::string kValid = "valid";
static const std::string kValue = "value";
static const std::string kWasUsed = "was_used";
static const std::string kPercent = "percent";
static const std::string kLimit = "limit";
static const std::string kSeries = "series";
static const std::string kValidAny = "valid_any";
}  // namespace Coupon

namespace PaymentTech {

static const std::string kSumToPay = "sum_to_pay";
static const std::string kWithoutVatToPay = "without_vat_to_pay";
static const std::string kUserToPay = "user_to_pay";
static const std::string kType = "type";
static const std::string kPrevType = "prev_type";
static const std::string kPcv = "pcv";
static const std::string kMainCardBillingId = "main_card_billing_id";
static const std::string kMainCardPaymentId = "main_card_payment_id";
static const std::string kMainCardPersistentId = "main_card_persistent_id";
static const std::string kMainCardPossibleMoneyless =
    "main_card_possible_moneyless";
static const std::string kLastKnownIp = "last_known_ip";
static const std::string kDebt = "debt";
static const std::string kNeedCvn = "need_cvn";
static const std::string kNeedAccept = "need_accept";
static const std::string kCanTakeTips = "ctt";
static const std::string kAntifraudGroupRecalculated =
    "antifraud_group_recalculated";
static const std::string kAntifraudGroup = "antifraud_group";
static const std::string kCheckCardIsFinished = "check_card_is_finished";
static const std::string kAntifraudFinished = "antifraud_finished";
static const std::string kFinishHandled = "finish_handled";
static const std::string kNotifications = "notifications";
static const std::string kHoldInitiated = "hold_initiated";
static const std::string kHistory = "history";
static const std::string kBillingFallbackEnabled = "billing_fallback_enabled";

namespace Notifications {
static const std::string kUpdaterequest = "updaterequest";
static const std::string kPayment = "payment";

static const std::string kToSend = "to_send";
static const std::string kSent = "sent";
static const std::string kAttempts = "attempts";

namespace UpdateRequest {
static const std::string kToSend = "to_send";
static const std::string kSent = "sent";
static const std::string kAttempts = "attempts";

namespace Notification {
static const std::string kId = "id";
static const std::string kWarnNoCard = "warn_no_card";
static const std::string kWaitPaymentComplete = "wait_payment_complete";
static const std::string kPaidByCard = "paid_by_card";
static const std::string kVersion = "version";
static const std::string kStamp = "stamp";
}  // namespace Notification
}  // namespace UpdateRequest
}  // namespace Notifications

namespace ToPay {
static const std::string kRide = "ride";
static const std::string kTips = "tips";
}  // namespace ToPay

}  // namespace PaymentTech

namespace BillingTech {
static const std::string kVersion = "version";
static const std::string kServiceOrders = "service_orders";
static const std::string kRefreshAttemptsCount = "refresh_attempts_count";
static const std::string kTransactions = "transactions";
static const std::string kCompensations = "compensations";

namespace ServiceOrders {
static const std::string kRide = "ride";
static const std::string kTips = "tips";
}  // namespace ServiceOrders

namespace Transactions {
static const std::string kStatus = "status";
static const std::string kTrustPaymentId = "trust_payment_id";
static const std::string kWaitForCvn = "wait_for_cvn";
}  // namespace Transactions
}  // namespace BillingTech

namespace Feedback {
static const std::string kRating = "rating";
static const std::string kMqc = "mqc";
static const std::string kMsg = "msg";
static const std::string kAnswer = "answer";
static const std::string kCancelledReason = "cancelled_reason";
static const std::string kLowRatingReason = "low_rating_reason";
static const std::string kChoices = "choices";
static const std::string kCreated = "ct";
static const std::string kIsAfterComplete = "iac";
static const std::string kCallRequested = "c";
static const std::string kUpdated = "u";
static const std::string kAppComment = "app_comment";
}  // namespace Feedback

namespace Route {
static const std::string kDistance = "distance";
static const std::string kTime = "time";
}  // namespace Route

namespace ClientNotifications {
static const std::string kMovedToCash = "moved_to_cash";

namespace MovedToCash {
static const std::string kSent = "sent";
}  // namespace MovedToCash
}  // namespace ClientNotifications

}  // namespace Orders

namespace FeedbackChoices {
static const std::string kCollection = kDb + ".feedback_choices";

static const std::string kId = "_id";
}  // namespace FeedbackChoices

namespace Users {
static const std::string kCollection = kDb + ".users";

static const std::string kId = "_id";
static const std::string kYandexUid = "yandex_uid";
static const std::string kUberId = "uber_id";
static const std::string kYandexUuid = "yandex_uuid";
static const std::string kOldYandexUuid = "old_yandex_uuid";
static const std::string kCreated = "created";
static const std::string kUpdated = "updated";
static const std::string kYandexStaff = "yandex_staff";
static const std::string kPhone = "phone";
static const std::string kPhoneId = "phone_id";
static const std::string kAuthorized = "authorized";
static const std::string kYandexCookie = "yandex_cookie";
static const std::string kTokenOnly = "token_only";
static const std::string kAntifraud = "antifraud";
static const std::string kDeviceId = "device_id";
static const std::string kBannersEnabled = "banners_enabled";
static const std::string kBannersSeen = "banners_seen";
static const std::string kPromotions = "promotions";
static const std::string kSvoCommitOrders = "svo_commit_orders";
static const std::string kApplication = "application";
static const std::string kApplicationVersion = "application_version";
static const std::string kApnsToken = "apns_token";
static const std::string kGcmToken = "gcm_token";
static const std::string kBlockedTill = "blocked_till";
static const std::string kUserExperiments = "user_experiments";
}  // namespace Users

namespace UserEmails {
static const std::string kId = "_id";
static const std::string kConfirmed = "confirmed";
static const std::string kConfirmationCode = "confirmation_code";
static const std::string kCreated = "created";
static const std::string kEmail = "email";
static const std::string kUpdated = "updated";
}  // namespace UserEmails

namespace UserDevices {
static const std::string kCollection = kDb + ".user_devices";
static const std::string kBlockedTill = "blocked_till";
static const std::string kId = "_id";
}  // namespace UserDevices

namespace UserSettings {
static const std::string kCollection = kDb + ".user_settings";

static const std::string kPhoneId = "phone_id";
static const std::string kSettings = "settings";
}  // namespace UserSettings

namespace Cities {
static const std::string kCollection = kDb + ".cities";

static const std::string kId = "_id";
static const std::string kRequirements = "requirements";
static const std::string kCoupon = "coupon";
static const std::string kCountry = "country";
static const std::string kTimezone = "tz";

static const std::string kCantChangeDestination = "cant_change_destinations";
static const std::string kEng = "eng";
static const std::string kFeedbackOptions = "feedback_choices";
static const std::string kHolidays = "holidays";
static const std::string kWorkdays = "workdays";
static const std::string kCreditcard = "creditcard";
static const std::string kTopLeft = "tl";
static const std::string kBottomRight = "br";
static const std::string kExactOrders = "exact_orders";
static const std::string kGeocoderObjects = "geocoder_objects";
static const std::string kCenterTab = "center_tab";
static const std::string kCheckContracts = "check_contracts";
static const std::string kDisabled = "disabled";
}  // namespace Cities

namespace Countries {
static const std::string kCollection = kDb + ".countries";

static const std::string kId = "_id";
}  // namespace Countries

namespace FullscreenBanners {
static const std::string kCollection = kDb + ".fullscreen_banners";

}  // namespace FullscreenBanners

namespace Promotions {
static const std::string kCollection = kDb + ".promotions";

}  // namespace Promotions

namespace Geoareas {
static const std::string kCollection = kDb + ".geoareas";

static const std::string kCoordinates = "coordinates";
static const std::string kGeometry = "geometry";
static const std::string kGeoId = "geo_id";
static const std::string kId = "_id";
static const std::string kName = "name";
static const std::string kRemoved = "removed";
static const std::string kType = "t";
}  // namespace Geoareas

namespace Tariffs {
static const std::string kCollection = kDb + ".tariffs";

static const std::string kActivationZone = "activation_zone";
static const std::string kCategories = "categories";
static const std::string kCategoryCurrency = "currency";
static const std::string kIncludedOneOf = "included_one_of";
static const std::string kCategoryDayType = "dt";
static const std::string kCategoryId = "id";
static const std::string kCategoryMeters = "m";
static const std::string kCategoryMinimalPrice = "minimal";
static const std::string kCategoryName = "name";
static const std::string kCategoryOrderTimeTo = "order_to_time";
static const std::string kCategoryPaidDispatchDistanceIntervals = "pddpi";
static const std::string kCategoryRequirementPrices = "req_prices";
static const std::string kCategorySpecialTaximeters = "st";
static const std::string kCategoryTankerKey = "name_key";
static const std::string kCategoryTimeFrom = "from_time";
static const std::string kCategoryTimeTo = "to_time";
static const std::string kCategoryTransfers = "zp";
static const std::string kCategoryDynamicTransfers = "dynamic_transfers";
static const std::string kDateFrom = "date_from";
static const std::string kDateTo = "date_to";
static const std::string kHomeZone = "home_zone";
static const std::string kId = "_id";
static const std::string kIntervalBegin = "b";
static const std::string kIntervalEnd = "e";
static const std::string kIntervalMode = "m";
static const std::string kIntervalPrice = "p";
static const std::string kIntervalStep = "s";
static const std::string kMeterTriggerType = "s";
static const std::string kPriceObjectDistanceIntervals = "dpi";
static const std::string kPriceObjectDistanceMeterId = "dmi";
static const std::string kPriceObjectTimeIntervals = "tpi";
static const std::string kPriceObjectTimeMeterId = "tmi";
static const std::string kRelatedZones = "rz";
static const std::string kRequirementPricePrice = "p";
static const std::string kRequirementPriceType = "t";
static const std::string kSpecialTaximeterPrice = "p";
static const std::string kSpecialTaximeterZoneName = "z";
static const std::string kTransferDestinationZoneName = "dst";
static const std::string kTransferPriceObject = "p";
static const std::string kTransferPriceObjectMinimal = "minimal";
static const std::string kTransferPriceRouteWithoutJams = "route_without_jams";
static const std::string kTransferPriceObjectOnce = "once";
static const std::string kDynamicTransferPriceObjectCoefficient = "coeff";
static const std::string kTransferSourceZoneName = "src";
static const std::string kWaitingIncludedTime = "waiting";
static const std::string kWaitingPrice = "waiting_price";
static const std::string kWaitingPriceType = "waiting_price_type";
}  // namespace Tariffs

namespace TariffSettings {
static const std::string kCollection = kDb + ".tariff_settings";

static const std::string kId = "_id";
static const std::string kZoneName = "hz";
static const std::string kCityId = "city_id";
static const std::string kPaidCancelEnabled = "paid_cancel_enabled";
static const std::string kTimezone = "tz";
static const std::string kCountry = "country";
static const std::string kPaymentOptions = "payment_options";
static const std::string kCategories = "s";
static const std::string kHolidays = "holidays";
static const std::string kWorkdays = "workdays";
static const std::string kCheckContracts = "check_contracts";
static const std::string kFeedback_choices = "feedback_choices";
static const std::string kSurgeUpgradeType = "surge_upgrade_type";
static const std::string kSimpleSurgeUpgrade = "surge_tariff_class_upgrades";
static const std::string kRatioSurgeUpgrade = "surge_ratio_upgrade";
static const std::string kSurgeRatioRules = "surge_ratio";
static const std::string kPriceRatioRules = "price_ratio";
static const std::string kCancelledReason = "cancelled_reason";
namespace SurgeUpgradeTypes {
static const std::string kSimple = "simple";
static const std::string kRatio = "ratio";
}  // namespace SurgeUpgradeTypes

// Pool settings
static const std::string kDiscountDecrementPerPassenger = "ddpp";
static const std::string kPoolMinDiscount = "pmnd";
static const std::string kPoolMaxDiscount = "pmxd";
static const std::string kPoolDiscountQ = "pdq";
static const std::string kPoolDiscountFLat = "pdf";
static const std::string kPoolDiscountFlatEnabled = "pdfe";
static const std::string kPoolFalseNegative = "pfnp";

namespace Notification {
static const std::string kKey = "k";
static const std::string kType = "t";
static const std::string kShowCount = "c";
static const std::string kTranslations = "tr";
}  // namespace Notification

namespace Categories {
static const std::string kDefault = "d";
static const std::string kCanBeDefault = "r";
static const std::string kMarkAsNew = "mark_as_new";
static const std::string kCommentsDisabled = "comments_disabled";
static const std::string kOnlyForSoonOrders = "oso";
static const std::string kTankerKey = "t";
static const std::string kReqDestination = "rd";
static const std::string kRestrictByPaymentType = "rpt";
static const std::string kSubzones = "sz";
static const std::string kClientConstraints = "cc";
static const std::string kClientRequirements = "client_requirements";
static const std::string kPersistentRequirements = "persistent_reqs";
static const std::string kGluedRequirements = "glued_requirements";
static const std::string kIncludedOneOf = "included_one_of";
static const std::string kBrandingId = "branding_id";
static const std::string kClassName = "n";
static const std::string kServiceLevels = "sl";
static const std::string kSupportedRequirementsLabel = "srl";
static const std::string kSupportedRequirementsPlaceholder = "srpl";
static const std::string kDeferredOrdersEnabled = "doe";
static const std::string kMaxRoutePointsCount = "mrpc";
static const std::string kMaxWaitingTime = "mwt";
static const std::string kFreeCancelTimeout = "fct";
static const std::string kNotifications = "nos";
static const std::string kTransfersConfig = "tc";
static const std::string kRedirects = "redirects";

namespace Redirects {
static const std::string kRequirementFrom = "requirement_from";
static const std::string kRequirementTo = "requirement_to";
static const std::string kTariffClass = "tariff_class";
static const std::string kDescriptionKey = "description_key";
}  // namespace Redirects

namespace TransferConfig {
static const std::string kSource = "src";
static const std::string kDestinations = "dsts";
}  // namespace TransferConfig

}  // namespace Categories
}  // namespace TariffSettings

namespace Images {
static const std::string kCollection = kDb + ".images";

}  // namespace Images

namespace Parks {
static const std::string kCollection = kDb + ".parks";

static const std::string kId = "_id";
static const std::string kContracts = "account.contracts";
static const std::string kCorporateContracts = "account.corporate_contracts";
static const std::string kCorpVATs = "corp_vats";
static const std::string kRequirements = "requirements";

namespace Contracts {
static const std::string kIsCash = "is_of_cash";
static const std::string kIsCard = "is_of_card";
static const std::string kServices = "services";
static const std::string kCurrency = "currency";
}  // namespace Contracts

namespace CorporateContracts {
static const std::string kIsActive = "is_active";
}  // namespace CorporateContracts

namespace Requirements {
static const std::string kCard = "creditcard";
static const std::string kCorp = "corp";
static const std::string kCoupon = "coupon";
}  // namespace Requirements

}  // namespace Parks

namespace Drivers {
static const std::string kCollection = kDb + ".drivers";

static const std::string kId = "_id";
static const std::string kCar = "car";
static const std::string kDbId = "db_id";
static const std::string kName = "name";
static const std::string kUuid = "uuid";
static const std::string kLocale = "locale";
static const std::string kClass = "class";
static const std::string kAcceptanceRate = "acceptance_rate";
static const std::string kCompletedRate = "completed_rate";
static const std::string kDriverLicense = "driver_license";
static const std::string kUpdated = "updated";
static const std::string kState = "_state";

namespace Car {
static const std::string kColor = "color";
static const std::string kColorCode = "color_code";
static const std::string kModel = "model";
static const std::string kNumber = "number";
static const std::string kRawModel = "raw_model";
}  // namespace Car
}  // namespace Drivers

namespace UniqueDrivers {
static const std::string kCollection = kDb + ".unique_drivers";

static const std::string kId = "_id";
static const std::string kLicenses = "licenses";
static const std::string kLicensesLicense = "licenses.license";
}  // namespace UniqueDrivers

namespace UniqueDriverZoneStats {
static const std::string kCollection =
    kDbProcessing + ".unique_drivers_zone_stats";

static const std::string kId = "_id";
static const std::string kZoneName = "nz";
static const std::string kUniqueDriverId = "udi";
static const std::string kDate = "md";
static const std::string kTariffClass = "tc";
static const std::string kFullBrandingNumOrders = "nfbo";
static const std::string kStickerNumOrders = "nso";
static const std::string kLightboxNumOrders = "nlo";
static const std::string kNumOrders = "no";
static const std::string kUpdated = "u";

}  // namespace UniqueDriverZoneStats

namespace DriverPositions {
static const std::string kCollection = kDb + ".driver_positions";
static const std::string kId = "_id";
static const std::string kLastKnownZone = "last_known_zone";
}  // namespace DriverPositions

namespace Cards {
static const std::string kCollection = kDb + ".cards";

static const std::string kCreated = "created";
static const std::string kUpdated = "updated";
static const std::string kOwnerUid = "owner_uid";
static const std::string kOwnerName = "owner_name";
static const std::string kBillingId = "billing_id";
static const std::string kPersistentId = "persistent_id";
static const std::string kPaymentId = "payment_id";
static const std::string kCurrency = "currency";
static const std::string kNumber = "number";
static const std::string kSystem = "system";
static const std::string kBlockingReason = "blocking_reason";
static const std::string kRegionId = "region_id";
static const std::string kRegionsChecked = "regions_checked";
static const std::string kValid = "valid";
static const std::string kBusyWith = "busy_with";
static const std::string kOrderId = "order_id";
static const std::string kServiceLabels = "service_labels";
static const std::string kPossibleMoneyless = "possible_moneyless";
static const std::string kUnbound = "unbound";
static const std::string kExpirationDate = "expiration_date";

namespace RegionsChecked {
static const std::string kId = "id";
}  // namespace RegionsChecked

namespace BusyWith {
static const std::string kOrderId = "order_id";
}  // namespace BusyWith
}  // namespace Cards

namespace PersonalSubventions {
static const std::string kCollection = kDbProcessing + ".personal_subventions";

static const std::string kId = "_id";
static const std::string kUniqueDriverId = "udi";
static const std::string kRuleId = "ri";
static const std::string kStart = "s";
static const std::string kEnd = "e";
}  // namespace PersonalSubventions

namespace PromocodeSeries {
static const std::string kCollection = kDb + ".promocode_series";

static const std::string kId = "_id";
static const std::string kIsUnique = "is_unique";
static const std::string kFirstLimit = "first_limit";
static const std::string kUserLimit = "user_limit";
static const std::string kCount = "count";
static const std::string kUsagePerPromocode = "usage_per_promocode";
static const std::string kApplications = "applications";
static const std::string kCities = "cities";
static const std::string kZones = "zones";
static const std::string kValue = "value";
static const std::string kCurrency = "currency";
static const std::string kCountry = "country";
static const std::string kStart = "start";
static const std::string kFinish = "finish";
static const std::string kRequiresActivationAfter = "requires_activation_after";
static const std::string kCreditcardOnly = "creditcard_only";
static const std::string kPercent = "percent";
static const std::string kPercentLimitPerTrip = "percent_limit_per_trip";

namespace Race {
static const std::string kCountFull = "_race.count";
static const std::string kUserLimitFull = "_race.user_limit";
}  // namespace Race
}  // namespace PromocodeSeries

namespace PromocodeReferrals {
static const std::string kCollection = kDb + ".promocode_referrals";

static const std::string kId = "_id";
static const std::string kCreatorPhoneId = "creator_phone_id";
static const std::string kPromocodes = "promocodes";
static const std::string kSeqNo = "seq_no";

namespace Promocodes {
static const std::string kId = "_id";
static const std::string kCreated = "created";
static const std::string kValue = "value";
static const std::string kCurrency = "currency";
static const std::string kCountry = "country";
static const std::string kPercent = "percent";
static const std::string kActivations = "activations";
static const std::string kPercentLimitPerTrip = "percent_limit_per_trip";
static const std::string kRideCount = "ride_count";
static const std::string kIdFull = "promocodes._id";
}  // namespace Promocodes
}  // namespace PromocodeReferrals

namespace Promocodes {
static const std::string kCollection = kDb + ".promocodes";

static const std::string kId = "_id";
static const std::string kCode = "code";
static const std::string kSeriesId = "series_id";
static const std::string kPhoneId = "phone_id";
static const std::string kRevoked = "revoked";

namespace Revoked {
static const std::string kOperatorLogin = "operator_login";
static const std::string kOtrsTicket = "otrs_ticket";
static const std::string kCreated = "created";
}  // namespace Revoked
}  // namespace Promocodes

namespace PromocodeUsages {
static const std::string kCollection = kDb + ".promocode_usages";

static const std::string kId = "_id";
static const std::string kSeriesId = "series_id";
static const std::string kPhoneId = "phone_id";
static const std::string kReserve = "reserve";
static const std::string kValue = "value";
}  // namespace PromocodeUsages

namespace SubventionRules {
static const std::string kCollection = kDb + ".subvention_rules";
static const std::string kId = "_id";
static const std::string kTariffZone = "tariffzone";
static const std::string kClass = "class";
static const std::string kType = "type";
static const std::string kDayRideCount = "dayridecount";
static const std::string kDayRideCountDays = "dayridecount_days";
static const std::string kDayRideCountIsForAnyCategory =
    "dayridecount_is_for_any_category";
static const std::string kIsBonus = "is_bonus";
static const std::string kIsOnce = "is_once";
static const std::string kSum = "sum";
static const std::string kBrandingType = "branding_type";
static const std::string kStart = "start";
static const std::string kEnd = "end";
static const std::string kHour = "hour";
static const std::string kDayofweek = "dayofweek";
static const std::string kPaymentType = "paymenttype";
static const std::string kCurrency = "currency";
static const std::string kGroupId = "group_id";
static const std::string kTimeZone = "time_zone";
static const std::string kAcceptanceRate = "acceptance_rate";
static const std::string kCompletionRate = "completion_rate";
static const std::string kLog = "log";
static const std::string kDisplayInTaximeter = "display_in_taximeter";
static const std::string kShowGeoareaInTaximeter = "show_geoarea_in_taximeter";

namespace Log {
static const std::string kUpdated = "updated";
}  // namespace Log
}  // namespace SubventionRules

namespace SurgePriceZone {
static const std::string kCollection = kDb + ".surge_price_areas";

static const std::string kId = "_id";
static const std::string kGeo = "g";
static const std::string kZoneSize = "s";
static const std::string kName = "n";
static const std::string kIsActive = "is_active";
static const std::string kBaseClass = "t";
static const std::string kForced = "forced";
static const std::string kMethod = "method";
static const std::string kAreaPins = "area_pins";
static const std::string kMinCoeff = "min_coeff";
static const std::string kMinPinsCount = "min_pins_count";
static const std::string kCalc = "calc";
static const std::string kPins = "pins";
static const std::string kFree = "free";
static const std::string kToal = "total";
static const std::string kRules = "rules";
static const std::string kSurgeValue = "surge_value";
static const std::string kReason = "reason";
static const std::string kMethodConvertRatio = "method_convert_ratio";
static const std::string kTimeRules = "time_rules";
static const std::string kTimeFrom = "time_from";
static const std::string kTimeTo = "time_to";
static const std::string kSurgeRules = "surge_rules";
static const std::string kSurgeFormula = "surge_formula";
static const std::string kPfSlope = "pf_slope";
static const std::string kPfIntercept = "pf_intercept";
static const std::string kLinearDependencyFormula = "linear_dependency_formula";
static const std::string kLinearDependencyFormulaEnabled = "enabled";
static const std::string kFromClass = "from_class";
static const std::string kSurgeLinearCoeff = "surge_linear_coeff";
static const std::string kBalance = "balance";
static const std::string kMinPins = "min_pins";
static const std::string kMinTotal = "min_total";
static const std::string kfInit = "f_init";
static const std::string kfEqual = "f_equal";
static const std::string kfDeltaLeft = "f_delta_left";
static const std::string kfDeltaRight = "f_delta_right";
static const std::string kfLmbd = "lmbd";
static const std::string kAddFree = "add_free";
static const std::string kAddTotal = "add_total";
static const std::string kfsIntercept = "fs_intercept";
static const std::string kfsCoefChain = "fs_coef_chain";
static const std::string kfsCoefTotal = "fs_coef_total";
static const std::string kUtilizationForNonBase =
    "utilization_for_non_econom";  // old field name. new should be
                                   // utilization_for_non_base
static const std::string kDisableAntisurge = "disable_antisurge";
static const std::string kTableCoefPs = "table_coef_ps";
static const std::string kSmoothSurge = "smooth_surge";
static const std::string kSmoothSurgeEnable = "enable";
static const std::string kSmoothSurgeMaxJumpUp = "max_jump_up";
static const std::string kSmoothSurgeMaxJumpDown = "max_jump_up";
static const std::string kSmoothTimings = "smooth_timings";
static const std::string kSmoothTimingsKappaEta = "kappa_eta";
static const std::string kSmoothTimingsKappaEtr = "kappa_etr";
static const std::string kSmoothTimingsGamma = "gamma";
static const std::string kSmoothTimingsAddFree = "add_free";
static const std::string kSurchargeEnabled = "surcharge_enabled";
}  // namespace SurgePriceZone

namespace Autoincrement {
static const std::string kCollection = kDb + ".autoincrement";

static const std::string kId = "_id";
static const std::string kCurrent = "current";
static const std::string kReferralPromocodes = "referral_promocodes";
}  // namespace Autoincrement

namespace AntispamCounter {
static const std::string kCollection = kDb + ".antispam_counters";

static const std::string kId = "_id";
static const std::string kIgnoreTag = "ignore_tag";
static const std::string kUpdated = "updated";
static const std::string kAttemptTimestamps = "attempt_timestamps";
}  // namespace AntispamCounter

namespace CouponFraud {
static const std::string kCollection = kDb + ".coupon_frauders";

static const std::string kId = "_id";
static const std::string kCreated = "created";
static const std::string kCoupons = "coupons";
}  // namespace CouponFraud

namespace RfidLabels {
static const std::string kCollection = kDb + ".rfid_labels";

static const std::string kId = "_id";  // carnumber - номер машины.
static const std::string kTagbarcode = "tagbarcode";
static const std::string kCodecompany = "codecompany";
static const std::string kDataidentifier = "dataidentifier";
static const std::string kEpc = "epc";
static const std::string kTid = "tid";
static const std::string kTagtype = "tagtype";
static const std::string kRfid = "rfid";
static const std::string kPeriod = "period";
static const std::string kStatus = "status";
static const std::string kUpdated = "updated";
}  // namespace RfidLabels

namespace DistLock {
static const std::string kCollection = kDb + ".distlock";
static const std::string kId = "_id";
const std::string kLockedTill = "t";
const std::string kOwner = "o";
}  // namespace DistLock

namespace CronTasks {
static const std::string kCollection = kDb + ".cron_tasks";
static const std::string kId = "_id";
static const std::string kName = "name";
static const std::string kHostname = "hostname";
static const std::string kStartTime = "start_time";
static const std::string kEndTime = "end_time";
static const std::string kExecutionTime = "execution_time";
static const std::string kClockTime = "clock_time";
static const std::string kStatus = "status";
}  // namespace CronTasks

namespace VipUsers {
static const std::string kCollection = kDb + ".vip_users";
static const std::string kId = "_id";
}  // namespace VipUsers

}  // namespace Taxi

namespace Users {
static const std::string kDb = "dbusers";

namespace UserPhones {
static const std::string kCollection = kDb + ".user_phones";

static const std::string kId = "_id";
static const std::string kPhone = "phone";
static const std::string kPhoneType = "type";
static const std::string kLastOrderCityId = "last_order_city_id";
static const std::string kCreated = "created";
static const std::string kUpdated = "updated";
static const std::string kStat = "stat";
static const std::string kBlockedTill = "blocked_till";
static const std::string kMqc = "mqc";
static const std::string kYandexStaff = "yandex_staff";
static const std::string kTaxiStaff = "taxi_staff";
static const std::string kLastPaymentMethod = "last_payment_method";

namespace Stat {
static const std::string kTotal = "total";
static const std::string kComplete = "complete";
static const std::string kCompleteCard = "complete_card";
static const std::string kFake = "fake";
static const std::string kCompleteApple = "complete_apple";
static const std::string kBigFirst = "big_first_discounts";
}  // namespace Stat

namespace LastPaymentMethod {
static const std::string kType = "type";
static const std::string kId = "id";
}  // namespace LastPaymentMethod

}  // namespace UserPhones

}  // namespace Users

namespace PinStats {
static const std::string kDb = "dbpinstats";

namespace PinStats {
static const std::string kCollection = kDb + ".pin_stats";

static const std::string kAntisurge = "antisurge";
static const std::string kCreated = "created";
static const std::string kEstimatedWaiting = "estimated_waiting";
static const std::string kForcedSurges = "forced_surges";
static const std::string kForcedSurgesInfo = "forced_surges_info";
static const std::string kGeoHash = "geohash";
static const std::string kGeopoint = "geopoint";
static const std::string kGeopointB = "geopoint_b";
static const std::string kIsFake = "is_fake";
static const std::string kPositionAccuracy = "position_accuracy";
static const std::string kSurgeId = "surge_id";
static const std::string kUserId = "user_id";
static const std::string kPins = "pins";
static const std::string kFree = "free";
static const std::string kFreeChain = "free_chain";
static const std::string kTotal = "total";
static const std::string kCategory = "category";
static const std::string kValue = "value";
static const std::string kValueB = "value_b";
static const std::string kValueRaw = "value_raw";
static const std::string kValueSmooth = "value_smooth";
static const std::string kValueSmoothB = "value_smooth_b";
static const std::string kRadius = "radius";
static const std::string kTR = "tr";
static const std::string kBL = "bl";
static const std::string kExperiment = "experiment";
static const std::string kFDerivative = "f_derivative";
static const std::string kPs = "ps";
static const std::string kCost = "cost";
static const std::string kTime = "time";
static const std::string kDistance = "distance";
static const std::string kEta = "eta";
static const std::string kEtr = "etr";
static const std::string kSurchargeAlpha = "surcharge_alpha";
static const std::string kSurchargeBeta = "surcharge_beta";
static const std::string kSurcharge = "surcharge";
}  // namespace PinStats

namespace SurgePins {
static const std::string kCollection = kDb + ".surge_pins";

static const std::string kUserId = "_id";
static const std::string kCreated = "c";
static const std::string kGeopoint = "g";
}  // namespace SurgePins
}  // namespace PinStats

namespace Processing {
static const std::string kDb = "dbprocessing";
static const std::string kVersion = "version";
static const std::string kNeedStart = "need_start";

namespace Lookup {
static const std::string kGeneration = "generation";
static const std::string kGenerationUpdated = "generation_updated";
static const std::string kStartEta = "eta";
static const std::string kRestartEta = "restart_eta";
static const std::string kNeedStart = "need_start";
static const std::string kState = "state";
}  // namespace Lookup

namespace OrderProc {
static const std::string kCollection = kDb + ".order_proc";

static const std::string kAliases = "aliases";
static const std::string kId = "_id";
static const std::string kCandidates = "candidates";
static const std::string kChanges = "changes";
static const std::string kDestinationsStatuses = "destinations_statuses";
static const std::string kLookup = "lookup";
static const std::string kOrder = "order";
static const std::string kPerformer = "performer";
static const std::string kReorder = "reorder";
static const std::string kStatus = "status";
static const std::string kCreated = "created";
static const std::string kUpdated = "updated";
static const std::string kChatId = "chat_id";
static const std::string kProcessing = "processing";
static const std::string kOrderInfo = "order_info";

namespace status {
static const std::string kPending = "pending";
static const std::string kAssigned = "assigned";
}  // namespace status

namespace Aliases {
static const std::string kId = "id";
static const std::string kDue = "due";
}  // namespace Aliases

namespace Candidates {
static const std::string kAliasId = "alias_id";
static const std::string kCp = "cp";
static const std::string kDriverId = "driver_id";
static const std::string kTaximeterVersion = "taximeter_version";
static const std::string kAcceptanceRate = "ar";
static const std::string kCompletedRate = "cr";
static const std::string kDue = "due";
static const std::string kId = "id";
static const std::string kDest = "dest";
}  // namespace Candidates

namespace Order {
static const std::string kCity = "city";
static const std::string kClientNotifications = "client_notifications";
static const std::string kExperiments = "experiments";
static const std::string kId = "_id";
static const std::string kNz = "nz";
static const std::string kPerformer = "performer";
static const std::string kRequest = "request";
static const std::string kStatus = "status";
static const std::string kPaymentTech = "payment_tech";
static const std::string kStatusUpdated = "status_updated";
static const std::string kTaxiStatus = "taxi_status";
static const std::string kType = "_type";
static const std::string kUserId = "user_id";
static const std::string kUserPhoneId = "user_phone_id";
static const std::string kYandexUid = "user_uid";
static const std::string kVersion = "version";
static const std::string kCreated = "created";
static const std::string kDeviceId = "device_id";
static const std::string kUserReady = "user_ready";
static const std::string kTerminalId = "terminal_id";
static const std::string kOperatorLogin = "operator_login";
static const std::string kReferralId = "referral_id";
static const std::string kReferralTimestamp = "referral_timestamp";
static const std::string kCost = "cost";
static const std::string kVendorId = "vendor_id";
static const std::string kUserLocale = "user_locale";
static const std::string kUserAgent = "user_agent";
static const std::string kDontCall = "dont_call";
static const std::string kDontSms = "dont_sms";
static const std::string kApplication = "application";
static const std::string kCoupon = "coupon";
static const std::string kRouteSharingKey = "rsk";
static const std::string kSvoCarNumber = "svo_car_number";
static const std::string kCalc = "calc";
static const std::string kFeedback = "feedback";
static const std::string kFixedPrice = "fixed_price";
static const std::string kDiscount = "discount";

namespace status {
static const std::string kPending = "pending";
}  // namespace status

namespace Coupon {
static const std::string kWasUsed = "was_used";
}  // namespace Coupon

namespace Performer {
static const std::string kClid = "clid";
static const std::string kUuid = "uuid";
static const std::string kDriverLicense = "driver_license";
static const std::string kPresetcar = "presetcar";
static const std::string kTariff = "tariff";

namespace Tariff {
static const std::string kClass = "class";
}  // namespace Tariff
}  // namespace Performer

namespace ClientNotifications {
static const std::string kMovedToCash = "moved_to_cash";

namespace MovedToCash {
static const std::string kSent = "sent";
}  // namespace MovedToCash
}  // namespace ClientNotifications

namespace Request {
static const std::string kSource = "source";
static const std::string kSourceRaw = "source_raw";
static const std::string kSp = "sp";
static const std::string kDestinations = "destinations";
static const std::string kClass = "class";
static const std::string kDue = "due";
static const std::string kPayment = "payment";

namespace Source {
static const std::string kGeopoint = "geopoint";
}  // namespace Source

namespace Payment {
static const std::string kType = "type";
}  // namespace Payment
}  // namespace Request
}  // namespace Order

namespace Request {
static const std::string kDue = "due";
}

namespace Reorder {
static const std::string kId = "id";
static const std::string kDecisions = "decisions";
static const std::string kSuggestions = "suggestions";

namespace Suggestion {
static const std::string kId = "id";
static const std::string kGeneration = "generation";
static const std::string kRequest = "request";
}  // namespace Suggestion

}  // namespace Reorder

namespace Performer {
static const std::string kCandidateIndex = "candidate_index";
static const std::string kAliasId = "alias_id";
static const std::string kDriverId = "driver_id";
static const std::string kNeedSync = "need_sync";
static const std::string kPresetcar = "presetcar";
static const std::string kParkId = "park_id";
}  // namespace Performer

namespace Processing {
static const std::string kNeedStart = "need_start";
static const std::string kVersion = "version";
}  // namespace Processing

namespace OrderInfo {
static const std::string kNeedSync = "need_sync";
static const std::string kWithCoupon = "with_coupon";
static const std::string kPassengersCount = "passengers_count";
static const std::string kLuggageCount = "luggage_count";
static const std::string kStatistics = "statistics";
static const std::string kSourcePoint = "source_point";
static const std::string kSubventionCalcRulesOld = "scr";

namespace Statistics {
static const std::string kSetCared = "setcared";
static const std::string kStatusUpdates = "status_updates";
static const std::string kRideReportSent = "ride_report_sent";
static const std::string kCorpRideReportSent = "corp_ride_report_sent";

static const std::string kCreated = "c";
static const std::string kStatus = "s";
static const std::string kReasonCode = "q";
static const std::string kNeedHandling = "h";
static const std::string kReasonArg = "a";
static const std::string kPosition = "p";
static const std::string kTaxiStatus = "t";

namespace Position {

static const std::string kUpdated = "updated";
static const std::string kTaxiStatus = "taxi_status";
static const std::string kPoint = "geopoint";
static const std::string kAvgSpeed = "avg_speed";
static const std::string kDirection = "direction";
static const std::string kAccuracy = "accuracy";
static const std::string kTime = "time";

}  // namespace Position
static const std::string kFromCabinet = "fc";
static const std::string kDriverPosition = "d";

namespace status {
static const std::string kPending = "pending";
}  // namespace status

namespace reason_code {
static const std::string kCreate = "create";
}  // namespace reason_code

}  // namespace Statistics
}  // namespace OrderInfo

namespace Changes {
static const std::string kObjects = "objects";
namespace Objects {
static const std::string kId = "id";
static const std::string kName = "n";
static const std::string kValue = "vl";
static const std::string kStatus = "s";

namespace Value {
static const std::string kPaymentMethodId = "payment_method_id";
}  // namespace Value
}  // namespace Objects
}  // namespace Changes
}  // namespace OrderProc

namespace Paf {
static const std::string kCreated = "c";
static const std::string kUpdated = "u";
static const std::string kMarker = "m";
static const std::string kRides = "r";
static const std::string kOrders = "o";
}  // namespace Paf

namespace RouteSharingKeys {
static const std::string kCollection = kDb + ".route_sharing_keys";

static const std::string kId = "_id";
static const std::string kKey = "k";
static const std::string kStartTime = "s";
static const std::string kUpdateTime = "u";
}  // namespace RouteSharingKeys
}  // namespace Processing

namespace Antifraud {
static const std::string kDb{"dbantifraud"};

namespace DriverDatamarts {
static const std::string kCollection{kDb + ".driver_datamarts"};
static const std::string kId{"_id"};
static const std::string kLicense{"license"};
static const std::string kWindow{"window"};
static const std::string kDoc{"doc"};
static const std::string kYtUpdate{"yt_update"};
}  // namespace DriverDatamarts

namespace PersonalSubventionRules {
static const std::string kCollection{kDb + ".personal_subvention_rules"};
static const std::string kId{"_id"};
static const std::string kSrc{"src"};
static const std::string kEnabled{"enabled"};
static const std::string kReasonMessage{"reason_message"};
static const std::string kDescription{"description"};
}  // namespace PersonalSubventionRules

namespace PersonalSubventionFrauders {
static const std::string kCollection{kDb + ".personal_subvention_frauders"};
static const std::string kId{"_id"};
static const std::string kLicense{"license"};
static const std::string kFound{"found"};
static const std::string kFrauder{"frauder"};
static const std::string kRuleId{"rule_id"};
static const std::string kRuleApply{"rule_apply"};
}  // namespace PersonalSubventionFrauders

namespace SubventionRules {
static const std::string kCollection{kDb + ".subvention_rules"};
static const std::string kId{"_id"};
static const std::string kSrc{"src"};
static const std::string kEnabled{"enabled"};
static const std::string kReasonMessage{"reason_message"};
static const std::string kPriority{"priority"};
static const std::string kDescription{"description"};
static const std::string kWindow{"window"};
static const std::string kCreated{"created"};
}  // namespace SubventionRules

namespace SubventionFrauders {
static const std::string kCollection{kDb + ".subvention_frauders"};
static const std::string kId{"_id"};
static const std::string kOrderId{"order_id"};
static const std::string kLicense{"license"};
static const std::string kFound{"found"};
static const std::string kFrauder{"frauder"};
static const std::string kRuleId{"rule_id"};
static const std::string kRuleApply{"rule_apply"};
static const std::string kConfidence{"confidence"};
}  // namespace SubventionFrauders

namespace CityInfo {
static const std::string kCollection{kDb + ".city_info"};
static const std::string kId{"_id"};
static const std::string kCity{"city"};
static const std::string kNumFirstRides{"num_first_rides"};
static const std::string kNumOrders{"num_orders"};
static const std::string kConvergence{"convergence"};
static const std::string kSolution{"solution"};
static const std::string kCreated{"created"};
static const std::string kUpdated{"updated"};
}  // namespace CityInfo

namespace EntityList {
static const std::string kCollection{kDb + ".entity_list"};
static const std::string kId{"_id"};
static const std::string kType{"type"};
}  // namespace EntityList

namespace EntityItem {
static const std::string kCollection{kDb + ".entity_item"};
static const std::string kId{"_id"};
static const std::string kListId{"list_id"};
static const std::string kValue{"value"};
static const std::string kCreated{"created"};
static const std::string kUpdated{"updated"};
}  // namespace EntityItem
}  // namespace Antifraud

namespace Logs {

static const std::string kDb = "dblogs";

namespace Servers {
static const std::string kCollection = kDb + ".servers";
}  // namespace Servers

namespace Responses {
static const std::string kCollection = kDb + ".servers_response";
}  // namespace Responses

}  // namespace Logs

namespace GprsTimings {

static const std::string kDb = "gprstimings";

namespace Geofences {
static const std::string kCollection = kDb + ".geofences";

static const std::string kId = "_id";
static const std::string kZones = "zones";
static const std::string kUpdated = "updated";
static const std::string kPoint = "point";
static const std::string kRadius = "radius";
static const std::string kMessage = "message";
}  // namespace Geofences

namespace Eta {
static const std::string kCollection = kDb + ".eta";

static const std::string kOrderId = "_id";
static const std::string kTimeLeft = "time_left";
static const std::string kUpdated = "updated";
static const std::string kErrorTime = "et";
}  // namespace Eta

namespace PickupGroups {
static const std::string kCollection = kDb + ".pickup_groups";

static const std::string kId = "_id";
static const std::string kName = "name";
static const std::string kZones = "zones";
static const std::string kPoints = "points";
static const std::string kAuthor = "author";
static const std::string kExperiment = "experiment";
static const std::string kEdited = "edited";
static const std::string kManual = "manual";
static const std::string kOverridden = "overridden";

namespace Points {
static const std::string kName = "name";
static const std::string kGeometry = "geometry";
static const std::string kScore = "score";
}  // namespace Points

namespace Zones {
static const std::string kName = "name";
static const std::string kGeometry = "geometry";
}  // namespace Zones
}  // namespace PickupGroups

}  // namespace GprsTimings

namespace UserCommunication {

static const std::string kDb{"dbuser_communication"};

namespace UserChatMessages {
static const std::string kCollection{kDb + ".user_chat_messages"};

static const std::string kVisible{"visible"};
static const std::string kOpen{"open"};
static const std::string kNewMessages{"new_messages"};
static const std::string kSupportAvatarUrl{"support_avatar_url"};
static const std::string kSupportName{"support_name"};
static const std::string kSupportTimestamp{"support_timestamp"};
static const std::string kMessages{"messages"};
static const std::string kSendPush{"send_push"};
static const std::string kUpdated{"updated"};
static const std::string kPhoneId{"user_phone_id"};
static const std::string kLastMessageFromUser{"last_message_from_user"};
static const std::string kAuthorId{"author_id"};
static const std::string kTicketId{"ticket_id"};

namespace Message {
static const std::string kId{"id"};
static const std::string kAuthor{"author"};
static const std::string kMessage{"message"};
static const std::string kMessageType{"message_type"};
static const std::string kTimestamp{"timestamp"};
}  // namespace Message

}  // namespace UserChatMessages

}  // namespace UserCommunication

}  // namespace db

}  // namespace mongo
}  // namespace storages
